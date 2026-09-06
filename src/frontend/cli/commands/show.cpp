#include "commands/show.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/colors.h"
#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

int command_show(int argc, char** argv) {
    struct {
        std::optional<uint32_t> id {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");
    parser.add_argument(args.id, "id").required(false).help("id of the entry");

    if (!parser.parse(argc, argv)) {
        return VAULT_GENERIC_ERROR;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    const std::filesystem::path vault_master_file_path = (vault_path / ".vault");

    const auto vault_password = secure_read_hidden_line_with_prompt("Vault password: ");
    if (!vault_password) {
        std::cerr << "ERROR: failed to load vault pw" << std::endl;
        return VAULT_GENERIC_ERROR;
    }

    const auto load_vault_result = load_vault(vault_master_file_path, *vault_password);
    if (!load_vault_result) {
        std::cerr << "ERROR: failed to load vault" << std::endl;
        return VAULT_GENERIC_ERROR;
    }

    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    if (args.id) {
        if (*args.id >= secrets_path.size()) {
            std::cerr << "ERROR: id " << *args.id << " does not exist" << std::endl;
            return VAULT_GENERIC_ERROR;
        }

        const auto& secret_path = secrets_path[*args.id];

        auto secret = load_secret(secret_path, *load_vault_result);
        if (!secret) {
            std::cerr << "ERROR: failed to load secret" << std::endl;
            return VAULT_GENERIC_ERROR;
        }

        std::cout << *args.id << ". " << CYAN << std::flush;
        secure_write(secret->name);
        std::cout << RESET << std::flush;

        secure_write(secret->content);
    } else {
        for (uint32_t i = 0; i < secrets_path.size(); i++) {
            const auto& secret_path = secrets_path[i];

            auto secret = load_secret(secret_path, *load_vault_result);
            if (!secret) {
                std::cerr << "ERROR: failed to load secret" << std::endl;
                return VAULT_GENERIC_ERROR;
            }

            std::cout << i << ". " << CYAN << std::flush;
            secure_write(secret->name);
            std::cout << RESET << std::flush;

            secure_write(secret->content);

            if (i != secrets_path.size() - 1) {
                std::cout << "\n";
            }
        }
    }

    return VAULT_SUCCESS;
}
