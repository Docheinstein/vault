#ifndef SECRET_H
#define SECRET_H

#include <filesystem>
#include <string>

#include "vault/common/retcodes.h"

// TODO
constexpr unsigned long long ENCRYPTION_SECRET_KEY_SIZE = 32;

VaultReturnCode save_secret_into_vault(const std::filesystem::path& vault_path, const std::string& name,
                                       const std::string& content, unsigned char secret_key[32],
                                       bool allow_replace = false);
VaultReturnCode load_secret(const std::filesystem::path& secret_path, unsigned char secret_key[32], std::string& name,
                            std::string& content);

#endif // SECRET_H
