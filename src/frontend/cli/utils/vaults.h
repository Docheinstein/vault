#ifndef TUI_VAULTSUTILS_H
#define TUI_VAULTSUTILS_H

#include <filesystem>
#include <vector>

#include "vault/vault/secret.h"

std::vector<std::string> get_all_secrets(const std::filesystem::path& vault_path);

#endif // TUI_VAULTSUTILS_H
