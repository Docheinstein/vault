#include "commands/init.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/env.h"
#include "utils/prompt.h"

int command_init(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    if (std::filesystem::exists(vault_path / ".vault")) {
        if (!read_yes_no_with_prompt("Vault already exists: overwrite? [y/N] ", false)) {
            return EXIT_SUCCESS;
        }
    }

    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        return EXIT_FAILURE;
    }

    const std::filesystem::path vault_master_file_path = (vault_path / ".vault");

    const auto save_vault_result = save_vault(vault_master_file_path, *vault_password);
    if (!save_vault_result) {
        std::cerr << "ERROR: failed to create vault" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
