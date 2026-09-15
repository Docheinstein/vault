#ifndef VAULTUTILS_H
#define VAULTUTILS_H

#include <filesystem>
#include <vector>

std::filesystem::path get_default_vault_path();

std::filesystem::path get_vault_master_file_path(const std::filesystem::path& vault_path);

std::vector<std::string> get_vault_secrets(const std::filesystem::path& vault_path);

std::string get_secret_short_name(const std::filesystem::path& secret_path);

#endif // VAULTUTILS_H
