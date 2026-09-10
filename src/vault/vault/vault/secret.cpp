#include "vault/vault/secret.h"

#include <filesystem>

#include "sodium.h"

#include "vault/fs/fileio.h"

/*
 * The secret file is file containing the name and the content
 * of the secret, as encrypted data.
 *
 * The header is not encrypted.
 *
 *  Scheme of the header.
 * +--------------------------------------------------+
 * |                Header (64 bytes)                |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Magic Bytes                 |    0:3  |       4  |
 * | Encryption Algorithm        |    4:4  |       1  |
 * | Unused                      |   5:15  |      11  |
 * |--------------------------------------------------|
 * | Encryption Data      (*1*)  |  16:63  |      48  |
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
 *
 * Scheme of the content.
 * +--------------------------------------------------+
 * |                Secret                            |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Name Length [L]            |     0:7  |       8  |
 * | Name                       | 8:8+L-1  |     [L]  |
 * | Content                    |   8+L:?  |   <var>  |
 * +--------------------------------------------------+
 *
 */

namespace {
// Subkey derivation.
constexpr unsigned long long KDF_CONTEXT_SIZE = crypto_kdf_CONTEXTBYTES;
constexpr unsigned long long KDF_NAME_SUBKEY_ID = 1;
constexpr char KDF_NAME_CONTEXT[KDF_CONTEXT_SIZE] = "NAME";
constexpr char KDF_CONTENT_CONTEXT[KDF_CONTEXT_SIZE] = "CONTENT";

// HMAC.
constexpr unsigned long long HMAC_KEY_SIZE = crypto_auth_KEYBYTES;
constexpr unsigned long long HMAC_OUTPUT_SIZE = crypto_auth_BYTES;

constexpr unsigned long long HMAC_OUTPUT_HEX_SIZE = HMAC_OUTPUT_SIZE * 2 + 1 /* \0 */;

// Encryption.
constexpr unsigned long long ENCRYPTION_MAC_SIZE = crypto_secretbox_MACBYTES;
constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = crypto_secretbox_KEYBYTES;
constexpr unsigned long long ENCRYPTION_NONCE_SIZE = crypto_secretbox_NONCEBYTES;
constexpr unsigned char ENCRYPTION_ALGO = 0;

// Secret file structure.
constexpr unsigned long long SECRET_MAGIC_BYTES_SIZE = 4;
constexpr unsigned char SECRET_MAGIC_BYTES[SECRET_MAGIC_BYTES_SIZE] = "SEC";

constexpr unsigned long long SECRET_HEADER_SIZE = 64;

constexpr unsigned long long SECRET_HEADER_PROLOGUE_BEGIN_POS = 0;
constexpr unsigned long long SECRET_HEADER_ENCRYPTION_BEGIN_POS = 16;

constexpr unsigned long long SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS = SECRET_HEADER_PROLOGUE_BEGIN_POS;
constexpr unsigned long long SECRET_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS =
    SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS + SECRET_MAGIC_BYTES_SIZE;

constexpr unsigned long long SECRET_HEADER_ENCRYPTION_NONCE_POS = SECRET_HEADER_ENCRYPTION_BEGIN_POS;

constexpr unsigned long long SECRET_CIPHERTEXT_POS = SECRET_HEADER_SIZE;

constexpr unsigned long long SECRET_NAME_SIZE_SIZE = 8;

constexpr unsigned long long SECRET_MINIMUM_FILE_SIZE =
    SECRET_HEADER_SIZE + ENCRYPTION_MAC_SIZE + SECRET_NAME_SIZE_SIZE;

auto authenticate_with_subkey(unsigned const char* plaintext, size_t length, const VaultKey& master_key,
                              uint64_t kdf_subkey_id, const char kdf_context[KDF_CONTEXT_SIZE]) {
    SecureArray<unsigned char, HMAC_KEY_SIZE> hmac_key {};
    crypto_kdf_derive_from_key(hmac_key.data(), hmac_key.size(), kdf_subkey_id, kdf_context, master_key.data());

    std::array<unsigned char, HMAC_OUTPUT_SIZE> hmac_output {};
    crypto_auth(hmac_output.data(), plaintext, length, hmac_key.data());

    return hmac_output;
}

uint64_t checksum(const unsigned char* buffer, const size_t length) {
    uint64_t h = 17;
    for (size_t i = 0; i < length; ++i) {
        h = h * 31 + buffer[i];
    }
    return h;
}
} // namespace

SaveSecretResult save_secret(const std::filesystem::path& vault_path, const VaultKey& vault_key, const Secret& secret,
                             bool allow_replace) {

    // Compute a unique name name for the secret.
    // The resulting name is deterministic, i.e. given the same secret key and plain name
    // the very same obscured name is produced. However, it can't be used to deduce or
    // retrieve the plain name of the secret (unless the secret key is known).
    const auto secret_name_obscure = authenticate_with_subkey(secret.name.data(), secret.name.size(), vault_key,
                                                              KDF_NAME_SUBKEY_ID, KDF_NAME_CONTEXT);

    // Get a hexadecimal representation of the HMAC.
    char secret_name_obscure_hex[HMAC_OUTPUT_HEX_SIZE];
    sodium_bin2hex(secret_name_obscure_hex, sizeof(secret_name_obscure_hex), secret_name_obscure.data(),
                   HMAC_OUTPUT_SIZE);

    // Generate a subkey id from the secret name's hex representation.
    const uint64_t kdf_secret_subkey_id =
        checksum(reinterpret_cast<const unsigned char*>(secret_name_obscure_hex), HMAC_OUTPUT_HEX_SIZE - 1);

    // Derive the secret key from the master key, using the subkey id associated with this secret name.
    VaultKey secret_key {};
    crypto_kdf_derive_from_key(secret_key.data(), secret_key.size(), kdf_secret_subkey_id, KDF_CONTENT_CONTEXT,
                               vault_key.data());

    // Generate random nonce for encryption.
    unsigned char encryption_nonce[ENCRYPTION_NONCE_SIZE];
    randombytes_buf(encryption_nonce, ENCRYPTION_NONCE_SIZE);

    // Build secret data.
    // The first 8 bytes contain the length of the secret name.
    // Then the name and the content are appended in this order.
    SecureString secret_plain_data {};
    secret_plain_data.reserve(secret.name.size() + secret.content.size() + SECRET_NAME_SIZE_SIZE);

    const uint64_t secret_name_size = secret.name.size();
    secret_plain_data.append(reinterpret_cast<const unsigned char*>(&secret_name_size), SECRET_NAME_SIZE_SIZE);
    secret_plain_data.append(secret.name);
    secret_plain_data.append(secret.content);

    // Encrypt using secret key (ChaCha20Poly1035).
    const unsigned long long output_size = SECRET_HEADER_SIZE + ENCRYPTION_MAC_SIZE + secret_plain_data.size();

    std::vector<unsigned char> secret_data {};
    secret_data.resize(output_size);

    unsigned char* const ciphertext = secret_data.data() + SECRET_HEADER_SIZE;

    if (crypto_secretbox_easy(ciphertext, secret_plain_data.data(), secret_plain_data.size(), encryption_nonce,
                              secret_key.data()) != 0) {
        return std::unexpected {SaveSecretError::EncryptionFailed};
    }

    // Write header (not encrypted).
    unsigned char* const header = secret_data.data();

    // Prologue (32 bytes).
    memset(header, 0, SECRET_HEADER_SIZE);
    memcpy(header + SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS, SECRET_MAGIC_BYTES, SECRET_MAGIC_BYTES_SIZE);

    header[SECRET_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS] = ENCRYPTION_ALGO;

    // Encryption Data (48 bytes).
    memcpy(header + SECRET_HEADER_ENCRYPTION_NONCE_POS, encryption_nonce, ENCRYPTION_NONCE_SIZE);

    std::filesystem::path secret_path = vault_path / std::string {secret_name_obscure_hex};

    // Save file.
    if (!allow_replace && std::filesystem::exists(secret_path)) {
        return std::unexpected {SaveSecretError::WriteError};
    }

    const auto result = write_binary_file(secret_path, secret_data.data(), secret_data.size());
    if (!result) {
        return std::unexpected {SaveSecretError::WriteError};
    }

    return {};
}

LoadSecretResult load_secret(const std::filesystem::path& secret_path, const VaultKey& vault_key) {
    // Load file.
    const ReadBinaryFileResult read_result = read_binary_file(secret_path.string());
    if (!read_result) {
        return std::unexpected {LoadSecretError::ReadError};
    }

    // File validity checks.
    if (read_result->size() <= SECRET_MINIMUM_FILE_SIZE) {
        return std::unexpected {LoadSecretError::InvalidFile};
    }

    const unsigned char* secret_data = read_result->data();
    const unsigned char* const ciphertext = secret_data + SECRET_CIPHERTEXT_POS;
    const unsigned char* const nonce = secret_data + SECRET_HEADER_ENCRYPTION_NONCE_POS;

    const unsigned long long ciphertext_size = read_result->size() - SECRET_HEADER_SIZE;

    const auto secret_obscure_file_name = secret_path.filename().string();

    // Generate a subkey id from the secret name's hex representation.
    const uint64_t kdf_secret_subkey_id = checksum(
        reinterpret_cast<const unsigned char*>(secret_obscure_file_name.data()), secret_obscure_file_name.size());

    // Derive the secret key from the master key, using the subkey id associated with this secret name.
    VaultKey secret_key {};
    crypto_kdf_derive_from_key(secret_key.data(), ENCRYPTION_SECRET_KEY_SIZE, kdf_secret_subkey_id, KDF_CONTENT_CONTEXT,
                               vault_key.data());

    // Decrypt using secret key (ChaCha20Poly1035).
    SecureString output {};
    output.resize(ciphertext_size - ENCRYPTION_MAC_SIZE);

    if (crypto_secretbox_open_easy(output.data(), ciphertext, ciphertext_size, nonce, secret_key.data()) != 0) {
        return std::unexpected {LoadSecretError::DecryptionFailed};
    }

    // Retrieve the length of the secret name from the first 8 bytes.
    uint64_t secret_name_size = 0;
    memcpy(&secret_name_size, output.data(), SECRET_NAME_SIZE_SIZE);

    // Build the secret by retrieving the name and the content from the data.
    Secret secret {};
    secret.name.append(output.data() + SECRET_NAME_SIZE_SIZE, secret_name_size);

    const size_t secret_content_size = output.size() - SECRET_NAME_SIZE_SIZE - secret_name_size;
    secret.content.append(output.data() + SECRET_NAME_SIZE_SIZE + secret_name_size, secret_content_size);

    return secret;
}

bool is_secret_file(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    const auto magic_bytes_result = read_binary_file(path.string(), SECRET_MAGIC_BYTES_SIZE);
    if (!magic_bytes_result.has_value()) {
        return false;
    }

    return std::memcmp(magic_bytes_result->data(), SECRET_MAGIC_BYTES, SECRET_MAGIC_BYTES_SIZE) == 0;
}
