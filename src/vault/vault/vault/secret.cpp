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

std::vector<unsigned char> hmac(const char* plaintext, unsigned char secret_key[32], uint16_t subkey_id,
                                const char ctx[crypto_kdf_CONTEXTBYTES]) {
    std::vector<unsigned char> output {};
    output.resize(crypto_auth_BYTES);

    unsigned char hmac_key[crypto_auth_KEYBYTES];
    crypto_kdf_derive_from_key(hmac_key, sizeof(hmac_key), subkey_id, ctx, secret_key);

    crypto_auth(output.data(), reinterpret_cast<const unsigned char*>(plaintext), strlen(plaintext), hmac_key);

    return output;
}

uint64_t get_subkey_id_from_name(const std::string& secret_name) {
    uint64_t subkey_id = 17;
    for (const auto c : secret_name) {
        subkey_id = (subkey_id * 31 + c);
    }
    return subkey_id;
}

} // namespace

VaultReturnCode save_secret_into_vault(const std::filesystem::path& vault_path, const std::string& name,
                                       const std::string& content, unsigned char secret_key[32], bool allow_replace) {

    // Compute secret obscured name using hmac
    const auto secret_name_obscure_result = hmac(name.c_str(), secret_key, 1, "NAME   ");

    constexpr size_t SECRET_NAME_OBSCURE_HEX_SIZE = crypto_auth_BYTES * 2 + 1; // +1 for '\0'

    char secret_name_obscure_hex[SECRET_NAME_OBSCURE_HEX_SIZE];
    sodium_bin2hex(secret_name_obscure_hex, sizeof(secret_name_obscure_hex), secret_name_obscure_result.data(),
                   secret_name_obscure_result.size());

    const std::string secret_name_obscure {secret_name_obscure_hex};

    // decrpypt

    uint64_t subkey_id = get_subkey_id_from_name(secret_name_obscure);
    crypto_kdf_derive_from_key(secret_key, ENCRYPTION_SECRET_KEY_SIZE, subkey_id, "CONTENT", secret_key);

    // fill nonce with random bytes
    unsigned char encryption_nonce[ENCRYPTION_NONCE_SIZE];
    randombytes_buf(encryption_nonce, ENCRYPTION_NONCE_SIZE);

    const std::string secret_content = name + "\n" + content;
    const unsigned char* secret_content_data {reinterpret_cast<const unsigned char*>(secret_content.data())};
    auto secret_content_size = secret_content.size();

    // Encrypt using secret key (ChaCha20Poly1035).
    const unsigned long long ciphertext_length = secret_content_size + ENCRYPTION_MAC_SIZE;

    const unsigned long long output_length = SECRET_HEADER_SIZE + ciphertext_length;
    std::vector<unsigned char> output {};
    output.resize(output_length);

    unsigned char* const ciphertext = output.data() + SECRET_HEADER_SIZE;
    if (crypto_secretbox_easy(ciphertext, secret_content_data, secret_content_size, encryption_nonce, secret_key) !=
        0) {
        return VAULT_GENERIC_ERROR;
    }

    unsigned char* const header = output.data();

    // Prologue (32 bytes).
    memset(header, 0, SECRET_HEADER_SIZE);
    memcpy(header + SECRET_HEADER_PROLOGUE_MAGIC_BYTES_POS, SECRET_MAGIC_BYTES, SECRET_MAGIC_BYTES_SIZE);

    header[SECRET_HEADER_PROLOGUE_ENCRYPTION_ALGO_POS] = ENCRYPTION_ALGO;

    // Encryption Data (48 bytes).
    memcpy(header + SECRET_HEADER_ENCRYPTION_NONCE_POS, encryption_nonce, ENCRYPTION_NONCE_SIZE);

    std::filesystem::path secret_path = (vault_path / secret_name_obscure).string();

    if (!allow_replace && std::filesystem::exists(secret_path)) {
        return VAULT_GENERIC_ERROR;
    }

    const auto result = write_binary_file(secret_path, output.data(), output.size());
    if (!result) {
        return VAULT_GENERIC_ERROR;
    }

    return VAULT_SUCCESS;
}

VaultReturnCode load_secret(const std::filesystem::path& secret_path, unsigned char secret_key[32], std::string& name,
                            std::string& content) {
    const read_binary_file_result read_result = read_binary_file(secret_path.string());
    if (!read_result) {
        return VAULT_GENERIC_ERROR;
    }

    if (read_result->size() < SECRET_HEADER_SIZE) {
        return VAULT_GENERIC_ERROR;
    }

    unsigned char dec_secret_key[ENCRYPTION_SECRET_KEY_SIZE];

    uint64_t subkey_id = get_subkey_id_from_name(secret_path.filename().string());
    crypto_kdf_derive_from_key(dec_secret_key, sizeof(dec_secret_key), subkey_id, "CONTENT", secret_key);

    const unsigned char* const ciphertext = read_result->data() + SECRET_CIPHERTEXT_POS;

    const unsigned char* const nonce = read_result->data() + SECRET_HEADER_ENCRYPTION_NONCE_POS;

    // Decrypt using secret key (ChaCha20Poly1035).
    const unsigned long long ciphertext_length = read_result->size() - SECRET_HEADER_SIZE;
    std::vector<unsigned char> output {};
    output.resize(ciphertext_length - ENCRYPTION_MAC_SIZE);

    if (crypto_secretbox_open_easy(output.data(), ciphertext, ciphertext_length, nonce, dec_secret_key) != 0) {
        return VAULT_GENERIC_ERROR;
    }

    uint64_t cursor = 0;
    while (cursor < output.size() && output[cursor] != '\n') {
        ++cursor;
    }

    if (cursor == output.size()) {
        return VAULT_GENERIC_ERROR;
    }

    name = std::string {output.begin(), output.begin() + cursor};
    content = std::string {output.begin() + cursor + 1, output.end()};

    return VAULT_SUCCESS;
}
