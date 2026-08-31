#ifndef VAULT_H
#define VAULT_H

#include <string>

#include "vault/common/retcodes.h"

VaultReturnCode save_vault(const std::string& path, const std::string& password);

VaultReturnCode load_vault(const std::string& path, const std::string& password, unsigned char secret_key[32]);

#endif // VAULT_H
