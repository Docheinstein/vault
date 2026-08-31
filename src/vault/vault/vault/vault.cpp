#include "vault/vault/vault.h"

#include "sodium.h"

#include "simdjson.h"

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
 * | Encryption Algorithm        |    4:4  |       1  |
 * | Password Hash Algorithm     |    5:5  |       1  |
 * | Unused                      |   6:31  |      26  |
 * |--------------------------------------------------|
 * | Encryption Data      (*1*)  |  32:79  |      48  |
 * | Password Hash Data   (*2*)  | 80:127  |      48  |
 * +--------------------------------------------------+
 *
 *
 * +--------------------------------------------------+
 * | (*1*)        Encryption Data (48 bytes)          |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Nonce                        |  0:23  |      24  |
 * | Unused                       | 24:47  |      24  |
 * +--------------------------------------------------+
 *
 * +--------------------------------------------------+
 * | (*2*)      Password Hash Data (48 bytes)         |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Salt                        |   0:15  |      16  |
 * | Memory Limit                |  16:23  |       8  |
 * | Ops Limit                   |  24:24  |       1  |
 * | Unused                      |  25:47  |      23  |
 * +--------------------------------------------------+
 *
 */

namespace {
constexpr unsigned char ENCRYPTION_ALGO_SECRETBOX_EASY = 0;
constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = crypto_secretbox_KEYBYTES;
constexpr unsigned long long ENCRYPTION_MAC_SIZE = crypto_secretbox_MACBYTES;
constexpr unsigned long long ENCRYPTION_NONCE_SIZE = crypto_secretbox_NONCEBYTES;
constexpr unsigned char ENCRYPTION_ALGO = ENCRYPTION_ALGO_SECRETBOX_EASY;

constexpr unsigned long long MAXIMUM_PLAINTEXT_LENGTH = 256 * (1 << 20);

constexpr unsigned long long PWHASH_SALT_SIZE = crypto_pwhash_SALTBYTES;
constexpr unsigned long long PWHASH_OPSLIMIT = crypto_pwhash_OPSLIMIT_INTERACTIVE;
constexpr unsigned long long PWHASH_MEMLIMIT = crypto_pwhash_MEMLIMIT_INTERACTIVE;
constexpr unsigned char PWHASH_ALGO = crypto_pwhash_ALG_ARGON2ID13;

constexpr unsigned long long PWHASH_OPSLIMIT_SIZE = 1;
constexpr unsigned long long PWHASH_MEMLIMIT_SIZE = 8;

constexpr unsigned long long VAULT_MAGIC_BYTES_SIZE = 4;
constexpr unsigned char VAULT_MAGIC_BYTES[VAULT_MAGIC_BYTES_SIZE] = "VLT";

constexpr unsigned long long VAULT_HEADER_SIZE = 128;

constexpr unsigned long long VAULT_HEADER_PROLOGUE_BEGIN_POS = 0;
constexpr unsigned long long VAULT_HEADER_ENCRYPTION_BEGIN_POS = 16;
constexpr unsigned long long VAULT_HEADER_PWHASH_BEGIN_POS = VAULT_HEADER_ENCRYPTION_BEGIN_POS + 48;

constexpr unsigned long long VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS = VAULT_HEADER_PROLOGUE_BEGIN_POS;
constexpr unsigned long long VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS =
    VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS + VAULT_MAGIC_BYTES_SIZE;
constexpr unsigned long long VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS = VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS + 1;

constexpr unsigned long long VAULT_HEADER_ENCRYPTION_NONCE_POS = VAULT_HEADER_ENCRYPTION_BEGIN_POS;

constexpr unsigned long long VAULT_HEADER_PWHASH_SALT_POS = VAULT_HEADER_PWHASH_BEGIN_POS;
constexpr unsigned long long VAULT_HEADER_PWHASH_MEMLIMIT_POS = VAULT_HEADER_PWHASH_SALT_POS + PWHASH_SALT_SIZE;
constexpr unsigned long long VAULT_HEADER_PWHASH_OPSLIMIT_POS = VAULT_HEADER_PWHASH_MEMLIMIT_POS + PWHASH_MEMLIMIT_SIZE;

constexpr unsigned long long VAULT_CIPHERTEXT_POS = VAULT_HEADER_SIZE;

constexpr unsigned long long MAXIMUM_PASSWORD_LENGTH = 128;

void read_header_prologue(const unsigned char* const header, unsigned char* const magic,
                          unsigned char* const pwhash_algo, unsigned char* const encryption_algo) {
    memcpy(magic, header + VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS, VAULT_MAGIC_BYTES_SIZE);
    *pwhash_algo = header[VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS];
    *encryption_algo = header[VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS];
}
} // namespace

VaultReturnCode save_vault(const std::string& path, const std::string& password) {
    unsigned char pwhash_salt[PWHASH_SALT_SIZE];
    unsigned char encryption_secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    unsigned char encryption_nonce[ENCRYPTION_NONCE_SIZE];

    // Check that password is safe.
    const unsigned long long password_length = strnlen(password.c_str(), MAXIMUM_PASSWORD_LENGTH);
    if (password_length >= MAXIMUM_PASSWORD_LENGTH) {
        return VAULT_GENERIC_ERROR;
    }

    // Generate random salt.
    randombytes_buf(pwhash_salt, sizeof(pwhash_salt));

    // Generate secret key from password (Argon2).
    if (crypto_pwhash_argon2id(encryption_secret_key, sizeof(encryption_secret_key), password.c_str(), password_length,
                               pwhash_salt, PWHASH_OPSLIMIT, PWHASH_MEMLIMIT, PWHASH_ALGO) != 0) {
        return VAULT_GENERIC_ERROR;
    }

    // Generate nonce for encryption.
    randombytes_buf(encryption_nonce, sizeof(encryption_nonce));

    // Encrypt using secret key (ChaCha20Poly1035).
    const unsigned long long ciphertext_length = ENCRYPTION_MAC_SIZE;

    const unsigned long long output_length = VAULT_HEADER_SIZE + ciphertext_length;

    std::vector<unsigned char> output {};
    output.resize(output_length);

    unsigned char* const ciphertext = output.data() + VAULT_HEADER_SIZE;
    if (crypto_secretbox_easy(ciphertext, nullptr, 0, encryption_nonce, encryption_secret_key) != 0) {
        return VAULT_GENERIC_ERROR;
    }

    unsigned char* const header = output.data();

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

    const auto result = write_binary_file(path, output.data(), output.size());
    if (!result) {
        return VAULT_GENERIC_ERROR;
    }

    return VAULT_SUCCESS;
}

VaultReturnCode load_vault(const std::string& path, const std::string& password,
                           unsigned char secret_key[ENCRYPTION_SECRET_KEY_SIZE]) {

    const read_binary_file_result read_result = read_binary_file(path);
    if (!read_result) {
        return VAULT_GENERIC_ERROR;
    }

    if (read_result->size() < VAULT_HEADER_SIZE) {
        return VAULT_GENERIC_ERROR;
    }

    // Check that password is safe.
    const unsigned long long password_length = strnlen(password.c_str(), MAXIMUM_PASSWORD_LENGTH);
    if (password_length >= MAXIMUM_PASSWORD_LENGTH) {
        return VAULT_GENERIC_ERROR;
    }
    unsigned char magic[VAULT_MAGIC_BYTES_SIZE];
    unsigned char pwhash_algo;
    unsigned char encryption_algo;

    const unsigned char* data = read_result->data();

    read_header_prologue(data, magic, &pwhash_algo, &encryption_algo);

    if (memcmp(magic, VAULT_MAGIC_BYTES, VAULT_MAGIC_BYTES_SIZE) != 0) {
        return VAULT_GENERIC_ERROR;
    }

    const unsigned char* const salt = data + VAULT_HEADER_PWHASH_SALT_POS;
    const unsigned char* const nonce = data + VAULT_HEADER_ENCRYPTION_NONCE_POS;

    const unsigned char* const memlimit = data + VAULT_HEADER_PWHASH_MEMLIMIT_POS;
    const unsigned char* const opslimit = data + VAULT_HEADER_PWHASH_OPSLIMIT_POS;
    const unsigned char* const ciphertext = data + VAULT_CIPHERTEXT_POS;

    unsigned long long opslimit_val {};
    memcpy(&opslimit_val, opslimit, PWHASH_OPSLIMIT_SIZE);

    unsigned long long memlimit_val {};
    memcpy(&memlimit_val, memlimit, PWHASH_MEMLIMIT_SIZE);

    // Generate secret key from password.
    if (crypto_pwhash(secret_key, ENCRYPTION_SECRET_KEY_SIZE, password.c_str(), password_length, salt, opslimit_val,
                      memlimit_val, pwhash_algo) != 0) {
        return VAULT_GENERIC_ERROR;
    }

    // Decrypt using secret key (ChaCha20Poly1035).
    std::vector<unsigned char> output {};

    const unsigned long long ciphertext_length = read_result->size() - VAULT_HEADER_SIZE;
    output.resize(ciphertext_length - ENCRYPTION_MAC_SIZE);

    if (crypto_secretbox_open_easy(output.data(), ciphertext, ciphertext_length, nonce, secret_key) != 0) {
        return VAULT_GENERIC_ERROR;
    }

    return VAULT_SUCCESS;
}
