#include "commands/create.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "ui/prompt.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_create(int argc, char** argv) {
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

    std::cout << "Leave the field empty to terminate the insertion and create the vault" << std::endl;

    do {
        Vault::Field field {};
        std::cout << "Name: " << std::flush;
        field.name = get_text();
        if (field.name.empty()) {
            break;
        }

        std::cout << "Hidden [y/N]? " << std::flush;
        field.hidden = read_yes_no_answer(false);

        vault.fields.push_back(std::move(field));
    } while (true);

    if (vault.fields.empty()) {
        std::cerr << "ERROR: failed to create vault '" << vault_name << "': it must have at least a field" << std::endl;
        exit(EXIT_EXECUTION_FAILED);
    }

    const std::string password = read_password();

    std::filesystem::create_directories(vaults_path);

    save_vault(vault, vault_path, password);
}
