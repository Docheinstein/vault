#include "commands/update.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "ui/prompt.h"

#include "utils/display.h"
#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_update(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_name {};
        std::optional<uint32_t> id {};
        std::optional<std::string> vaults_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_name, "name").required(false).help("vault name");
    parser.add_argument(args.id, "id").required(false).help("id of the entry");
    parser.add_argument(args.vaults_path, "--path").required(false).help("vaults path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::string vault_name = args.vault_name.has_value() ? *args.vault_name : read_vault_name();
    const std::filesystem::path vaults_path =
        args.vaults_path.has_value() ? std::filesystem::path {*args.vaults_path} : get_default_vaults_path();
    const std::filesystem::path vault_path = vaults_path / vault_name;

    const uint64_t id = args.id.has_value() ? args.id.value() : read_id();

    const std::string password = read_password();

    Vault vault {};
    load_vault(vault, vault_path, password);

    const uint64_t entry_index = entry_id_to_index(vault, id);

    Vault::Entry& entry = vault.entries[entry_index];

    std::cout << "Select the field to update." << std::endl;
    uint32_t field_cursor = 0;
    for (const auto& field : vault.fields) {
        std::cout << std::to_string(field_cursor + 1) << ". " << field.name << " ("
                  << get_entry_field_value(vault, entry, field_cursor) << ")" << std::endl;
        field_cursor++;
    }

    const uint32_t field_number = read_number_with_prompt("Field: ");
    const uint32_t field_index = field_number_to_index(vault, field_number);
    check_field_index_for_entry(vault, entry, field_index);

    std::cout << vault.fields[field_index].name << ": " << std::flush;
    std::string new_value = vault.fields[field_index].hidden ? get_hidden_text() : get_text();

    entry.values[field_index] = std::move(new_value);

    save_vault(vault, vault_path, password);
}