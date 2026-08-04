#include "vault/crypto/crypto.h"

#include <cstring>

#include "sodium.h"

#include "vault/fs/fileio.h"

/*
 * Scheme of the header.
 *
 * +--------------------------------------------------+
 * |                Header (128 bytes)                |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Magic Bytes                 |    0:3  |       4  |
 * | Password Hash Algorithm     |    4:4  |       1  |
 * | Encryption Algorithm        |    5:5  |       1  |
 * | Unused                      |   6:31  |      26  |
 * |--------------------------------------------------|
 * | Password Hash Data   (*1*)  |  32:79  |      48  |
 * | Encryption Data      (*2*)  | 80:127  |      48  |
 * +--------------------------------------------------+
 *
 *
 * +--------------------------------------------------+
 * | (*1*)      Password Hash Data (48 bytes)         |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Salt                        |   0:15  |      16  |
 * | Memory Limit                |  16:23  |       8  |
 * | Ops Limit                   |  24:24  |       1  |
 * | Unused                      |  25:47  |      23  |
 * +--------------------------------------------------+
 *
 *
 * +--------------------------------------------------+
 * | (*2*)        Encryption Data (48 bytes)          |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Nonce                        |  0:23  |      24  |
 * | Unused                       | 24:47  |      24  |
 * +--------------------------------------------------+
 */

namespace {
constexpr unsigned long long MAXIMUM_PASSWORD_LENGTH = 128;
constexpr unsigned long long MAXIMUM_PLAINTEXT_LENGTH = 256 * (1 << 20);

constexpr unsigned long long PWHASH_SALT_SIZE = crypto_pwhash_SALTBYTES;
constexpr unsigned long long PWHASH_OPSLIMIT = crypto_pwhash_OPSLIMIT_INTERACTIVE;
constexpr unsigned long long PWHASH_MEMLIMIT = crypto_pwhash_MEMLIMIT_INTERACTIVE;
constexpr unsigned char PWHASH_ALGO = crypto_pwhash_ALG_ARGON2ID13;

constexpr unsigned long long PWHASH_OPSLIMIT_SIZE = 1;
constexpr unsigned long long PWHASH_MEMLIMIT_SIZE = 8;

constexpr unsigned char ENCRYPTION_ALGO_SECRETBOX_EASY = 0;
constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = crypto_secretbox_KEYBYTES;
constexpr unsigned long long ENCRYPTION_MAC_SIZE = crypto_secretbox_MACBYTES;
constexpr unsigned long long ENCRYPTION_NONCE_SIZE = crypto_secretbox_NONCEBYTES;
constexpr unsigned char ENCRYPTION_ALGO = ENCRYPTION_ALGO_SECRETBOX_EASY;

constexpr unsigned long long VAULT_MAGIC_BYTES_SIZE = 4;
constexpr unsigned char VAULT_MAGIC_BYTES[VAULT_MAGIC_BYTES_SIZE] = "VLT";

constexpr unsigned long long VAULT_HEADER_SIZE = 128;

constexpr unsigned long long VAULT_HEADER_PROLOGUE_BEGIN_POS = 0;
constexpr unsigned long long VAULT_HEADER_PWHASH_BEGIN_POS = 32;
constexpr unsigned long long VAULT_HEADER_ENCRYPTION_BEGIN_POS = 80;

constexpr unsigned long long VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS = VAULT_HEADER_PROLOGUE_BEGIN_POS;
constexpr unsigned long long VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS =
    VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS + VAULT_MAGIC_BYTES_SIZE;
constexpr unsigned long long VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS = VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS + 1;

constexpr unsigned long long VAULT_HEADER_PWHASH_SALT_POS = VAULT_HEADER_PWHASH_BEGIN_POS;
constexpr unsigned long long VAULT_HEADER_PWHASH_MEMLIMIT_POS = VAULT_HEADER_PWHASH_SALT_POS + PWHASH_SALT_SIZE;
constexpr unsigned long long VAULT_HEADER_PWHASH_OPSLIMIT_POS = VAULT_HEADER_PWHASH_MEMLIMIT_POS + PWHASH_MEMLIMIT_SIZE;

constexpr unsigned long long VAULT_HEADER_ENCRYPTION_NONCE_POS = VAULT_HEADER_ENCRYPTION_BEGIN_POS;
constexpr unsigned long long VAULT_HEADER_ENCRYPTION_CIPHERTEXT_POS = VAULT_HEADER_SIZE;

void write_header(unsigned char* const header, const unsigned char* const pwhash_salt,
                  const unsigned char* const encryption_nonce) {
    // Prologue (32 bytes).
    memset(header, 0, VAULT_HEADER_SIZE);
    memcpy(header + VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS, VAULT_MAGIC_BYTES, VAULT_MAGIC_BYTES_SIZE);

    header[VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS] = PWHASH_ALGO;
    header[VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS] = ENCRYPTION_ALGO;

    // Password Hash Data (48 bytes).
    memcpy(header + VAULT_HEADER_PWHASH_SALT_POS, pwhash_salt, PWHASH_SALT_SIZE);
    memcpy(header + VAULT_HEADER_PWHASH_MEMLIMIT_POS, &PWHASH_MEMLIMIT, 8);
    header[VAULT_HEADER_PWHASH_OPSLIMIT_POS] = PWHASH_OPSLIMIT;

    // Encryption Data (48 bytes).
    memcpy(header + VAULT_HEADER_ENCRYPTION_NONCE_POS, encryption_nonce, ENCRYPTION_NONCE_SIZE);
}

void read_header_prologue(const unsigned char* const header, unsigned char* const magic,
                          unsigned char* const pwhash_algo, unsigned char* const encryption_algo) {
    memcpy(magic, header + VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS, VAULT_MAGIC_BYTES_SIZE);
    *pwhash_algo = header[VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS];
    *encryption_algo = header[VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS];
}
} // namespace

encrypt_result encrypt(const char* const plaintext, const char* const password) {
    std::vector<unsigned char> output {};

    // Check that password is safe.
    const unsigned long long password_length = strnlen(password, MAXIMUM_PASSWORD_LENGTH);
    if (password_length == MAXIMUM_PASSWORD_LENGTH) {
        return std::unexpected {EncryptError::WrongPassword};
    }

    // Generate random salt.
    unsigned char pwhash_salt[PWHASH_SALT_SIZE];
    randombytes_buf(pwhash_salt, sizeof(pwhash_salt));

    // Generate secret key from password (Argon2).
    unsigned char encryption_secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    if (crypto_pwhash_argon2id(encryption_secret_key, sizeof(encryption_secret_key), password, password_length,
                               pwhash_salt, PWHASH_OPSLIMIT, PWHASH_MEMLIMIT, PWHASH_ALGO) != 0) {
        return std::unexpected {EncryptError::PasswordHashGenerationFailed};
    }

    // Generate nonce for encryption.
    unsigned char encryption_nonce[ENCRYPTION_NONCE_SIZE];
    randombytes_buf(encryption_nonce, sizeof(encryption_nonce));

    // Encrypt using secret key (ChaCha20Poly1035).
    const unsigned long long plaintext_length = strnlen(plaintext, MAXIMUM_PLAINTEXT_LENGTH);
    if (plaintext_length == MAXIMUM_PLAINTEXT_LENGTH) {
        return std::unexpected {EncryptError::ContentTooLong};
    }

    const unsigned long long ciphertext_length = plaintext_length + ENCRYPTION_MAC_SIZE;

    const unsigned long long output_length = VAULT_HEADER_SIZE + ciphertext_length;
    output.resize(output_length);

    unsigned char* const ciphertext = output.data() + VAULT_HEADER_SIZE;
    if (crypto_secretbox_easy(ciphertext, reinterpret_cast<const unsigned char*>(plaintext), plaintext_length,
                              encryption_nonce, encryption_secret_key) != 0) {
        return std::unexpected {EncryptError::EncryptionFailed};
    }

    write_header(output.data(), pwhash_salt, encryption_nonce);

    return output;
}

decrypt_result decrypt(const std::vector<unsigned char>& input, const char* const password) {
    std::vector<unsigned char> output {};

    const unsigned long long password_length = strnlen(password, MAXIMUM_PASSWORD_LENGTH);
    if (password_length == MAXIMUM_PASSWORD_LENGTH) {
        return std::unexpected {DecryptError::WrongPassword};
    }

    if (input.size() < VAULT_HEADER_SIZE + ENCRYPTION_MAC_SIZE) {
        return std::unexpected {DecryptError::InvalidContent};
    }

    unsigned char magic[VAULT_MAGIC_BYTES_SIZE];
    unsigned char pwhash_algo;
    unsigned char encryption_algo;

    read_header_prologue(input.data(), magic, &pwhash_algo, &encryption_algo);

    if (memcmp(magic, VAULT_MAGIC_BYTES, VAULT_MAGIC_BYTES_SIZE) != 0) {
        return std::unexpected {DecryptError::InvalidContent};
    }

    const unsigned char* const salt = input.data() + VAULT_HEADER_PWHASH_SALT_POS;
    const unsigned char* const nonce = input.data() + VAULT_HEADER_ENCRYPTION_NONCE_POS;

    const unsigned char* const ciphertext = input.data() + VAULT_HEADER_ENCRYPTION_CIPHERTEXT_POS;
    const unsigned char* const memlimit = input.data() + VAULT_HEADER_PWHASH_MEMLIMIT_POS;
    const unsigned char* const opslimit = input.data() + VAULT_HEADER_PWHASH_OPSLIMIT_POS;

    unsigned long long opslimit_val {};
    memcpy(&opslimit_val, opslimit, PWHASH_OPSLIMIT_SIZE);

    unsigned long long memlimit_val {};
    memcpy(&memlimit_val, memlimit, PWHASH_MEMLIMIT_SIZE);

    // Generate secret key from password.
    unsigned char secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    if (crypto_pwhash(secret_key, ENCRYPTION_SECRET_KEY_SIZE, password, password_length, salt, opslimit_val,
                      memlimit_val, pwhash_algo) != 0) {
        return std::unexpected {DecryptError::PasswordHashGenerationFailed};
    }

    // Decrypt using secret key (ChaCha20Poly1035).
    const unsigned long long ciphertext_length = input.size() - VAULT_HEADER_SIZE;
    output.resize(ciphertext_length - ENCRYPTION_MAC_SIZE);

    if (crypto_secretbox_open_easy(output.data(), ciphertext, ciphertext_length, nonce, secret_key) != 0) {
        return std::unexpected {DecryptError::DecryptionFailed};
    }

    return output;
}
