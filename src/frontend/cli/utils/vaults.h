#ifndef VAULTSUTILS_H
#define VAULTSUTILS_H

#include <filesystem>
#include <vector>

std::filesystem::path get_default_vault_path();

std::filesystem::path get_vault_master_file_path(const std::filesystem::path& vault_path);

std::vector<std::string> get_vault_secrets(const std::filesystem::path& vault_path);

#endif // VAULTSUTILS_H
