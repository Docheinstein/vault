#ifndef SECRET_H
#define SECRET_H

#include <expected>
#include <filesystem>

#include "vault.h"

struct Secret {
    SecureString name {};
    SecureString content {};
};

enum class LoadSecretError { ReadError, InvalidFile, DecryptionFailed };

enum class SaveSecretError { WriteError, EncryptionFailed };

using LoadSecretResult = std::expected<Secret, LoadSecretError>;
using SaveSecretResult = std::expected<void, SaveSecretError>;

SaveSecretResult save_secret(const std::filesystem::path& vault_path, const VaultKey& vault_key, const Secret& secret,
                             bool allow_replace = false);
LoadSecretResult load_secret(const std::filesystem::path& secret_path, const VaultKey& vault_key);

bool is_secret_file(const std::filesystem::path& path);

#endif // SECRET_H
