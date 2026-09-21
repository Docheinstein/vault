#ifndef VAULTUTILS_H
#define VAULTUTILS_H

#include <filesystem>
#include <vector>

#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

std::filesystem::path get_default_vault_path();

std::filesystem::path get_vault_master_file_path(const std::filesystem::path& vault_path);

std::vector<std::string> get_vault_secrets(const std::filesystem::path& vault_path);

std::optional<std::string> get_secret_by_identifier(const std::filesystem::path& vault_path,
                                                    const std::string& identifier);

std::string get_secret_short_name(const std::filesystem::path& secret_path, uint32_t name_length = 6);

std::vector<std::pair<std::string, Secret>> load_identified_vault_secrets(const std::filesystem::path& vault_path,
                                                                          const VaultKey& vault_key,
                                                                          bool sort_by_name = false);

#endif // VAULTUTILS_H
