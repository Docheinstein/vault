#include "vault/vault/vault.h"

#include "sodium.h"

#include "vault/fs/fileio.h"
#include "vault/secure/securestring.h"

/*
 * The vault file is a header-only file, with no actual content.
 *
 * Its sole purpose is being used to verify the correctness
 * of a user-submitted password by decrypting it with a
 * secret key derived from the password.
 *
 * The header is not encrypted.
 *
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
 */

namespace {
// Key derivation.
constexpr unsigned long long PWHASH_SALT_SIZE = crypto_pwhash_SALTBYTES;
constexpr unsigned long long PWHASH_OPSLIMIT = crypto_pwhash_OPSLIMIT_INTERACTIVE;
constexpr unsigned long long PWHASH_MEMLIMIT = crypto_pwhash_MEMLIMIT_INTERACTIVE;
constexpr unsigned char PWHASH_ALGO = crypto_pwhash_ALG_ARGON2ID13;

constexpr unsigned long long PWHASH_OPSLIMIT_SIZE = 1;
constexpr unsigned long long PWHASH_MEMLIMIT_SIZE = 8;

// Encryption.
constexpr unsigned long long ENCRYPTION_MAC_SIZE = crypto_secretbox_MACBYTES;
constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = crypto_secretbox_KEYBYTES;
constexpr unsigned long long ENCRYPTION_NONCE_SIZE = crypto_secretbox_NONCEBYTES;
constexpr unsigned char ENCRYPTION_ALGO = 0;

// Vault file structure.
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

constexpr unsigned long long VAULT_FILE_SIZE = VAULT_HEADER_SIZE + ENCRYPTION_MAC_SIZE;

// Other.
constexpr unsigned long long MAXIMUM_PASSWORD_LENGTH = 128; // Arbitrary value.
} // namespace

SaveVaultResult save_vault(const std::filesystem::path& path, const SecureString& password) {
    // Password checks.
    if (password.size() > MAXIMUM_PASSWORD_LENGTH) {
        return std::unexpected {SaveVaultError::PasswordTooLong};
    }

    // Generate random salt for key derivation.
    unsigned char pwhash_salt[PWHASH_SALT_SIZE];
    randombytes_buf(pwhash_salt, sizeof(pwhash_salt));

    // Derive secret key from password (Argon2).
    VaultKey encryption_secret_key {};
    if (crypto_pwhash(encryption_secret_key.data(), VaultKey::Size, reinterpret_cast<char*>(password.data()),
                      password.size(), pwhash_salt, PWHASH_OPSLIMIT, PWHASH_MEMLIMIT, PWHASH_ALGO) != 0) {
        return std::unexpected {SaveVaultError::KeyDerivationFailed};
    }

    // Generate random nonce for encryption.
    unsigned char encryption_nonce[ENCRYPTION_NONCE_SIZE];
    randombytes_buf(encryption_nonce, sizeof(encryption_nonce));

    // Encrypt using secret key (ChaCha20Poly1035).
    constexpr unsigned long long output_size = VAULT_HEADER_SIZE + ENCRYPTION_MAC_SIZE;

    std::vector<unsigned char> vault_data {};
    vault_data.resize(output_size);

    unsigned char* const ciphertext = vault_data.data() + VAULT_HEADER_SIZE;

    if (crypto_secretbox_easy(ciphertext, nullptr, 0, encryption_nonce, encryption_secret_key.data()) != 0) {
        return std::unexpected {SaveVaultError::EncryptionFailed};
    }

    // Write header (not encrypted).
    unsigned char* const header = vault_data.data();

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

    // Save file.
    const auto result = write_binary_file(path, vault_data.data(), vault_data.size());
    if (!result) {
        return std::unexpected {SaveVaultError::WriteError};
    }

    return {};
}

LoadVaultResult load_vault(const std::filesystem::path& path, const SecureString& password) {
    // Load file.
    const ReadBinaryFileResult read_result = read_binary_file(path);
    if (!read_result) {
        return std::unexpected {LoadVaultError::ReadError};
    }

    // File validity checks.
    if (read_result->size() != VAULT_FILE_SIZE) {
        return std::unexpected {LoadVaultError::InvalidFile};
    }

    // Password checks.
    if (password.size() >= MAXIMUM_PASSWORD_LENGTH) {
        return std::unexpected {LoadVaultError::PasswordTooLong};
    }

    const unsigned char* vault_data = read_result->data();
    const unsigned char* header = vault_data;
    const unsigned char* const ciphertext = vault_data + VAULT_CIPHERTEXT_POS;

    const unsigned long long ciphertext_size = read_result->size() - VAULT_HEADER_SIZE;

    // Read header prologue.
    unsigned char magic[VAULT_MAGIC_BYTES_SIZE];
    memcpy(magic, header + VAULT_HEADER_PROLOGUE_MAGIC_BYTES_POS, VAULT_MAGIC_BYTES_SIZE);

    unsigned char pwhash_algo = header[VAULT_HEADER_PROLOGUE_PWHASH_ALGO_POS];
    unsigned char encryption_algo = header[VAULT_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS];

    // File content validity checks.
    if (encryption_algo != ENCRYPTION_ALGO) {
        return std::unexpected {LoadVaultError::UnexpectedFileParams};
    }

    if (memcmp(magic, VAULT_MAGIC_BYTES, VAULT_MAGIC_BYTES_SIZE) != 0) {
        return std::unexpected {LoadVaultError::InvalidFile};
    }
    const unsigned char* const pwhash_salt = vault_data + VAULT_HEADER_PWHASH_SALT_POS;
    const unsigned char* const encryption_nonce = vault_data + VAULT_HEADER_ENCRYPTION_NONCE_POS;

    unsigned long long pwhash_memlimit {};
    memcpy(&pwhash_memlimit, vault_data + VAULT_HEADER_PWHASH_MEMLIMIT_POS, PWHASH_MEMLIMIT_SIZE);

    unsigned long long pwhash_opslimit {};
    memcpy(&pwhash_opslimit, vault_data + VAULT_HEADER_PWHASH_OPSLIMIT_POS, PWHASH_OPSLIMIT_SIZE);

    VaultKey encryption_secret_key {};

    // Derive secret key from password (Argon2).
    if (crypto_pwhash(encryption_secret_key.data(), ENCRYPTION_SECRET_KEY_SIZE,
                      reinterpret_cast<const char*>(password.data()), password.size(), pwhash_salt, pwhash_opslimit,
                      pwhash_memlimit, pwhash_algo) != 0) {
        return std::unexpected {LoadVaultError::KeyDerivationFailed};
    }

    // Decrypt using secret key (ChaCha20Poly1035).
    if (crypto_secretbox_open_easy(nullptr, ciphertext, ciphertext_size, encryption_nonce,
                                   encryption_secret_key.data()) != 0) {
        return std::unexpected {LoadVaultError::DecryptionFailed};
    }

    return encryption_secret_key;
}
