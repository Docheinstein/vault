#ifndef CRYPTO_H
#define CRYPTO_H

#include <expected>
#include <vector>

enum class EncryptError {
    WrongPassword,
    PasswordHashGenerationFailed,
    ContentTooLong,
    EncryptionFailed,
};

enum class DecryptError {
    WrongPassword,
    InvalidContent,
    PasswordHashGenerationFailed,
    ContentTooLong,
    DecryptionFailed,
};

using encrypt_result = std::expected<std::vector<unsigned char>, EncryptError>;
using decrypt_result = std::expected<std::vector<unsigned char>, DecryptError>;

encrypt_result encrypt(const char* plaintext, const char* password);
decrypt_result decrypt(const std::vector<unsigned char>& input, const char* password);

#endif // CRYPTO_H
