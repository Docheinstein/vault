#ifndef TUI_VAULTSUTILS_H
#define TUI_VAULTSUTILS_H

#include <filesystem>

#include "vault/vault/vault.h"

void load_vault(Vault& vault, const std::filesystem::path& vault_path, const std::string& password);
void save_vault(const Vault& vault, const std::filesystem::path& vault_path, const std::string& password);

uint32_t entry_id_to_index(const Vault& vault, uint32_t id);
uint32_t field_number_to_index(const Vault& vault, uint32_t id);

std::optional<uint32_t> get_field_index(const Vault& vault, const std::string& field_name);

void check_field_index_for_vault(const Vault& vault, uint32_t field_index);
void check_field_index_for_entry(const Vault& vault, const Vault::Entry& entry, uint32_t field_index);

#endif // TUI_VAULTSUTILS_H
