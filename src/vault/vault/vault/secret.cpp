#include "vault/vault/secret.h"

#include <filesystem>

#include "sodium.h"

#include "simdjson.h"

#include "vault/fs/fileio.h"

/*
 * Scheme of the header.
 *
 * +--------------------------------------------------+
 * |                Header (64 bytes)                |
 * |--------------------------------------------------|
 * | Description                 |  Range  | # Bytes  |
 * |--------------------------------------------------|
 * | Magic Bytes                 |    0:3  |       4  |
 * | Encryption Algorithm        |    4:4  |       1  |
 * | Unused                      |   5:16  |      11  |
 * |--------------------------------------------------|
 * | Encryption Data      (*1*)  |  32:79  |      48  |
 * +--------------------------------------------------+
 *
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
constexpr unsigned char ENCRYPTION_ALGO_SECRETBOX_EASY = 0;
// constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = crypto_secretbox_KEYBYTES;
constexpr unsigned long long ENCRYPTION_MAC_SIZE = crypto_secretbox_MACBYTES;
constexpr unsigned long long ENCRYPTION_NONCE_SIZE = crypto_secretbox_NONCEBYTES;
constexpr unsigned char ENCRYPTION_ALGO = ENCRYPTION_ALGO_SECRETBOX_EASY;

constexpr unsigned long long SECRET_MAGIC_BYTES_SIZE = 4;
constexpr unsigned char SECRET_MAGIC_BYTES[SECRET_MAGIC_BYTES_SIZE] = "VLT";

constexpr unsigned long long SECRET_HEADER_SIZE = 64;

constexpr unsigned long long SECRET_HEADER_PROLOGUE_BEGIN_POS = 0;
constexpr unsigned long long SECRET_HEADER_ENCRYPTION_BEGIN_POS = 16;

constexpr unsigned long long SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS = SECRET_HEADER_PROLOGUE_BEGIN_POS;
constexpr unsigned long long SECRET_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS =
    SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS + SECRET_MAGIC_BYTES_SIZE;

constexpr unsigned long long SECRET_HEADER_ENCRYPTION_NONCE_POS = SECRET_HEADER_ENCRYPTION_BEGIN_POS;

constexpr unsigned long long SECRET_CIPHERTEXT_POS = SECRET_HEADER_SIZE;

std::vector<unsigned char> hmac(unsigned const char* plaintext, size_t length, unsigned char secret_key[32],
                                uint16_t subkey_id, const char ctx[crypto_kdf_CONTEXTBYTES]) {
    std::vector<unsigned char> output {};
    output.resize(crypto_auth_BYTES);

    unsigned char hmac_key[crypto_auth_KEYBYTES];
    crypto_kdf_derive_from_key(hmac_key, sizeof(hmac_key), subkey_id, ctx, secret_key);

    crypto_auth(output.data(), plaintext, length, hmac_key);

    return output;
}

uint64_t get_subkey_id_from_name(const char* buffer, size_t length) {
    uint64_t subkey_id = 17;
    for (size_t i = 0; i < length; ++i) {
        subkey_id = (subkey_id * 31 + buffer[i]);
    }
    return subkey_id;
}

} // namespace

SaveSecretResult save_secret_into_vault(const std::filesystem::path& vault_path, const SecureKey& secret_key,
                                        const Secret& secret, bool allow_replace) {

    // Compute secret obscured name using hmac
    const auto secret_name_obscure_result = hmac(secret.name.data(), secret.name.size(), secret_key.data, 1, "NAME   ");

    constexpr size_t SECRET_NAME_OBSCURE_HEX_SIZE = crypto_auth_BYTES * 2 + 1; // +1 for '\0'

    char secret_name_obscure_hex[SECRET_NAME_OBSCURE_HEX_SIZE] {};
    sodium_bin2hex(secret_name_obscure_hex, sizeof(secret_name_obscure_hex), secret_name_obscure_result.data(),
                   secret_name_obscure_result.size());

    // decrpypt
    std::string secret_name_obscure_hex_str {secret_name_obscure_hex};

    uint64_t subkey_id =
        get_subkey_id_from_name(secret_name_obscure_hex_str.data(), secret_name_obscure_hex_str.size());
    // std::cout << "Save secret subkey " << subkey_id << std::endl;

    SecureKey secret_key_derived;
    crypto_kdf_derive_from_key(secret_key_derived.data, ENCRYPTION_SECRET_KEY_SIZE, subkey_id, "CONTENT",
                               secret_key.data);

    // fill nonce with random bytes
    unsigned char encryption_nonce[ENCRYPTION_NONCE_SIZE];
    randombytes_buf(encryption_nonce, ENCRYPTION_NONCE_SIZE);

    // const std::string secret_content = name + "\n" + content;
    // const unsigned char* secret_content_data {reinterpret_cast<const unsigned char*>(secret_content.data())};
    SecureString secret_chained_content {};
    secret_chained_content.append(secret.name);
    secret_chained_content.append(reinterpret_cast<unsigned const char*>("\n"), 1);
    secret_chained_content.append(secret.content);

    // std::cout << "secret_chained_content.size() " << secret_chained_content.size() << std::endl;

    auto secret_content_size = secret_chained_content.size();

    // Encrypt using secret key (ChaCha20Poly1035).
    const unsigned long long ciphertext_length = secret_content_size + ENCRYPTION_MAC_SIZE;

    const unsigned long long output_length = SECRET_HEADER_SIZE + ciphertext_length;
    SecureString output {};
    output.resize(output_length);

    // std::cout << "ciphertext_length= " << ciphertext_length << std::endl;

    // std::cout << "Encrypted using data " << std::string{reinterpret_cast<const char*>(secret_key_derived.data), 32}
    // << subkey_id << std::endl;

    unsigned char* const ciphertext = output.data() + SECRET_HEADER_SIZE;

    if (crypto_secretbox_easy(ciphertext, secret_chained_content.data(), secret_chained_content.size(),
                              encryption_nonce, secret_key_derived.data) != 0) {
        return std::unexpected {SaveSecretError::GenericError};
    }

    unsigned char* const header = output.data();

    // Prologue (32 bytes).
    memset(header, 0, SECRET_HEADER_SIZE);
    memcpy(header + SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS, SECRET_MAGIC_BYTES, SECRET_MAGIC_BYTES_SIZE);

    header[SECRET_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS] = ENCRYPTION_ALGO;

    // Encryption Data (48 bytes).
    memcpy(header + SECRET_HEADER_ENCRYPTION_NONCE_POS, encryption_nonce, ENCRYPTION_NONCE_SIZE);

    std::filesystem::path secret_path = (vault_path / std::string {secret_name_obscure_hex}).string();

    if (!allow_replace && std::filesystem::exists(secret_path)) {
        return std::unexpected {SaveSecretError::GenericError};
    }

    const auto result = write_binary_file(secret_path, output.data(), output.size());
    if (!result) {
        return std::unexpected {SaveSecretError::GenericError};
    }

    return {};
}

LoadSecretResult load_secret(const std::filesystem::path& secret_path, const SecureKey& secret_key) {
    const ReadBinaryFileResult read_result = read_binary_file(secret_path.string());
    if (!read_result) {
        return std::unexpected {LoadSecretError::GenericError};
    }

    if (read_result->size() < SECRET_HEADER_SIZE) {
        return std::unexpected {LoadSecretError::GenericError};
    }

    SecureKey dec_secret_key;

    const auto secret_obscure_file_name = secret_path.filename().string();

    uint64_t subkey_id = get_subkey_id_from_name(secret_obscure_file_name.data(), secret_obscure_file_name.size());
    // std::cout << "Load secret subkey " << subkey_id << std::endl;

    crypto_kdf_derive_from_key(dec_secret_key.data, ENCRYPTION_SECRET_KEY_SIZE, subkey_id, "CONTENT", secret_key.data);

    const unsigned char* const ciphertext = read_result->data() + SECRET_CIPHERTEXT_POS;

    const unsigned char* const nonce = read_result->data() + SECRET_HEADER_ENCRYPTION_NONCE_POS;

    // Decrypt using secret key (ChaCha20Poly1035).
    const unsigned long long ciphertext_length = read_result->size() - SECRET_HEADER_SIZE;
    // std::cout << "ciphertext_length= " << ciphertext_length << std::endl;

    SecureString output {};
    output.resize(ciphertext_length - ENCRYPTION_MAC_SIZE);
    // std::cout << "Decrypting using data " << std::string{reinterpret_cast<const char*>(dec_secret_key.data), 32} <<
    // subkey_id << std::endl;

    if (crypto_secretbox_open_easy(output.data(), ciphertext, ciphertext_length, nonce, dec_secret_key.data) != 0) {
        // std::cerr << "ERROR: crypto_secretbox_open_easy failed to decrypt secret" << std::endl;
        return std::unexpected {LoadSecretError::GenericError};
    }

    uint64_t cursor = 0;
    while (cursor < output.size() && output.data()[cursor] != '\n') {
        ++cursor;
    }

    if (cursor == output.size()) {
        return std::unexpected {LoadSecretError::GenericError};
    }
    // std::cout<< "Output size: " << output.size();

    Secret secret {};
    secret.name.resize(cursor);
    memcpy(secret.name.data(), output.data(), cursor);
    secret.content.resize(output.size() - cursor - 1);
    memcpy(secret.content.data(), output.data() + cursor + 1, output.size() - cursor - 1);

    // std::cout<< "Name size: " << secret.name.size();
    // std::cout<< "Content size: " << secret.content.size();

    return secret;
}
