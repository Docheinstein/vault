#ifndef VAULT_H
#define VAULT_H

#include <expected>
#include <string>

#include "vault/common/retcodes.h"
#include "vault/secure/securekey.h"
#include "vault/secure/securestring.h"

enum class LoadVaultError { GenericError };

enum class SaveVaultError { GenericError };

using LoadVaultResult = std::expected<SecureKey, LoadVaultError>;
using SaveVaultResult = std::expected<void, SaveVaultError>;

SaveVaultResult save_vault(const std::string& path, const SecureString& password);
LoadVaultResult load_vault(const std::string& path, const SecureString& password);

#endif // VAULT_H
