#ifndef VAULT_H
#define VAULT_H

#include <expected>
#include <filesystem>

#include "vault/secure/securearray.h"
#include "vault/secure/securestring.h"

enum class LoadVaultError : uint8_t {
    PasswordTooLong,
    ReadError,
    InvalidFile,
    UnexpectedFileParams,
    KeyDerivationFailed,
    DecryptionFailed
};

enum class SaveVaultError : uint8_t { PasswordTooLong, WriteError, KeyDerivationFailed, EncryptionFailed };

using VaultKey = SecureArray<unsigned char, 32>;

using LoadVaultResult = std::expected<VaultKey, LoadVaultError>;
using SaveVaultResult = std::expected<void, SaveVaultError>;

SaveVaultResult save_vault(const std::filesystem::path& path, const SecureString& password);
LoadVaultResult load_vault(const std::filesystem::path& path, const SecureString& password);

#endif // VAULT_H
