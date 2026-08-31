#include "commands/init.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_init(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    if (std::filesystem::exists(vault_path / ".vault")) {
        std::cerr << "ERROR: vault already exists" << std::endl;
        exit(EXIT_FAILURE);
    }

    const std::string password = read_hidden_text_with_prompt("Enter vault password: ");

    const bool ret = save_vault((vault_path / ".vault").string(), password);

    if (ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to create vault" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Ok" << std::endl;
}
