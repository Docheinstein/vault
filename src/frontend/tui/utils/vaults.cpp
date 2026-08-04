#include "utils/vaults.h"

#include <iostream>

#include "vault/utils/strings.h"

#include "commands/exitcodes.h"

void load_vault(Vault& vault, const std::filesystem::path& vault_path) {
    if (!vault.load(vault_path.string())) {
        std::cerr << "ERROR: failed to open vault '" << vault_path.filename() << "'" << std::endl;
        exit(EXIT_IO_ERROR);
    }
}

void save_vault(const Vault& vault, const std::filesystem::path& vault_path) {
    const bool ok = vault.save(vault_path.string());
    if (!ok) {
        std::cerr << "ERROR: failed to save vault '" << vault_path.filename() << "'" << std::endl;
        exit(EXIT_IO_ERROR);
    }
}

uint32_t entry_id_to_index(const Vault& vault, uint32_t id) {
    if (!id || id > vault.entries.size()) {
        std::cerr << "ERROR: invalid id '" << id << "'" << std::endl;
        exit(EXIT_EXECUTION_FAILED);
    }

    return id - 1;
}

uint32_t field_number_to_index(const Vault& vault, uint32_t id) {
    if (!id || id > vault.fields.size()) {
        std::cerr << "ERROR: invalid field number '" << id << "'" << std::endl;
        exit(EXIT_EXECUTION_FAILED);
    }

    return id - 1;
}

std::optional<uint32_t> get_field_index(const Vault& vault, const std::string& field_name) {
    if (field_name == "id") {
        return std::nullopt;
    }

    if (const auto sort_field = std::find_if(vault.fields.begin(), vault.fields.end(),
                                             [&field_name](const Vault::Field& field) {
                                                 return string_compare_case_insensitive(field.name, field_name);
                                             });
        sort_field != vault.fields.end()) {
        return sort_field - vault.fields.begin();
    }

    std::cerr << "ERROR: unknown field '" << field_name << "'" << std::endl;
    exit(EXIT_EXECUTION_FAILED);
}

void check_field_index_for_vault(const Vault& vault, uint32_t field_index) {
    if (field_index >= vault.fields.size()) {
        std::cerr << "ERROR: invalid field index '" << field_index << "'" << std::endl;
        exit(EXIT_EXECUTION_FAILED);
    }
}

void check_field_index_for_entry(const Vault& vault, const Vault::Entry& entry, uint32_t field_index) {
    if (field_index >= entry.values.size()) {
        std::cerr << "ERROR: entry has no value for field '" << vault.fields[field_index].name << "'" << std::endl;
        exit(EXIT_EXECUTION_FAILED);
    }
}
