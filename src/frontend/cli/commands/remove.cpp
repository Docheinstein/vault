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

void command_remove(int argc, char** argv) {
    struct {
        std::optional<uint32_t> id {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");
    parser.add_argument(args.id, "id").required(false).help("id of the entry");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    // Check vault key.
    const std::string vault_password = read_hidden_text_with_prompt("Enter vault password: ");

    const uint32_t id = args.id.has_value() ? args.id.value() : read_number_with_prompt("ID: ");

    unsigned char secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    const bool ret = load_vault((vault_path / ".vault").string(), vault_password, secret_key);

    if (ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to open vault" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    if (id >= secrets_path.size()) {
        std::cerr << "ERROR: id " << id << " does not exist" << std::endl;
        exit(EXIT_FAILURE);
    }

    const auto& secret_path = secrets_path[id];

    if (!std::filesystem::remove(secret_path)) {
        std::cerr << "ERROR: failed to remove secret with id " << id << std::endl;
        exit(EXIT_FAILURE);
    }
}
