#ifndef SECRET_H
#define SECRET_H

#include <expected>
#include <filesystem>

#include "vault/secure/securekey.h"
#include "vault/secure/securestring.h"

// TODO
constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = 32;

struct Secret {
    SecureString name {};
    SecureString content {};
};

enum class LoadSecretError { GenericError };

enum class SaveSecretError { GenericError };

using LoadSecretResult = std::expected<Secret, LoadSecretError>;
using SaveSecretResult = std::expected<void, SaveSecretError>;

SaveSecretResult save_secret_into_vault(const std::filesystem::path& vault_path, const SecureKey& secret_key,
                                        const Secret& secret, bool allow_replace = false);
LoadSecretResult load_secret(const std::filesystem::path& secret_path, const SecureKey& secret_key);

#endif // SECRET_H
