#include "commands/list.h"

#include <cstring>
#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/env.h"

#include "utils/colors.h"
#include "utils/prompt.h"

#include "utils/vaults.h"

int command_list(int argc, char** argv) {
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

    const std::filesystem::path vault_master_file_path = (vault_path / ".vault");

    // Check vault key.
    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        return EXIT_FAILURE;
    }

    const auto load_vault_result = load_vault(vault_master_file_path, *vault_password);
    if (!load_vault_result) {
        return EXIT_FAILURE;
    }

    if (!load_vault_result) {
        std::cerr << "ERROR: failed to open vault" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    for (uint32_t i = 0; i < secrets_path.size(); i++) {
        const auto& secret_path = secrets_path[i];

        auto secret = load_secret(secret_path, *load_vault_result);
        if (!secret) {
            return EXIT_FAILURE;
        }

        std::cout << i << ". " << CYAN << std::flush;
        secure_write(secret->name);
        std::cout << RESET << std::flush;
    }

    return EXIT_SUCCESS;
}
