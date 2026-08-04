#include "commands/add.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "ui/prompt.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_add(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_name {};
        std::optional<std::string> vaults_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_name, "name").required(false).help("vault name");
    parser.add_argument(args.vaults_path, "--path").required(false).help("vaults path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::string vault_name = args.vault_name.has_value() ? *args.vault_name : read_vault_name();
    const std::filesystem::path vaults_path =
        args.vaults_path.has_value() ? std::filesystem::path {*args.vaults_path} : get_default_vaults_path();
    const std::filesystem::path vault_path = vaults_path / vault_name;

    Vault vault {};
    load_vault(vault, vault_path);

    Vault::Entry entry {};

    for (const auto& field : vault.fields) {
        std::cout << field.name << ": " << std::flush;
        std::string field_value = field.hidden ? get_password() : get_text();
        entry.values.push_back(std::move(field_value));
    }

    vault.entries.push_back(std::move(entry));

    save_vault(vault, vault_path);
}
