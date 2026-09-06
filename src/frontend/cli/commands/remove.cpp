#include "commands/remove.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

int command_remove(int argc, char** argv) {
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

    // Check vault key.

    const uint32_t id = args.id.has_value() ? args.id.value() : read_number_with_prompt("ID: ");

    //
    // const auto secret_key_result = load_vault((vault_path / ".vault").string(), vault_password);
    //
    // if (!secret_key_result) {
    //     std::cerr << "ERROR: failed to open vault" << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    //
    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    if (id >= secrets_path.size()) {
        std::cerr << "ERROR: invalid id " << id << std::endl;
        return VAULT_GENERIC_ERROR;
    }

    const auto& secret_path = secrets_path[id];

    if (!std::filesystem::remove(secret_path)) {
        std::cerr << "ERROR: failed to remove secret with id " << id << std::endl;
        return VAULT_GENERIC_ERROR;
    }

    return VAULT_SUCCESS;
}
