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

#include "commands/exitcodes.h"

void command_list(int argc, char** argv) {
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

    // Check vault key.
    const std::string vault_password = read_hidden_text_with_prompt("Enter vault password: ");

    unsigned char secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    const bool ret = load_vault((vault_path / ".vault").string(), vault_password, secret_key);

    if (ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to open vault" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    for (uint32_t i = 0; i < secrets_path.size(); i++) {
        const auto& secret_path = secrets_path[i];

        std::string secret_name {};
        std::string secret_content {};

        load_secret(secret_path, secret_key, secret_name, secret_content);

        std::cout << i << ". " << bold(cyan(secret_name)) << std::endl;
    }
}
