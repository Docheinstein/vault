#include "commands/remove.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_remove(int argc, char** argv) {
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

    const uint32_t id = args.id.has_value() ? args.id.value() : read_id();

    const std::string password = read_password();

    Vault vault {};
    load_vault(vault, vault_path, password);

    const uint32_t entry_index = entry_id_to_index(vault, id);

    const auto it = vault.entries.begin() + entry_index;

    vault.entries.erase(it);

    save_vault(vault, vault_path, password);
}
