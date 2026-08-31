#include "commands/edit.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_edit(int argc, char** argv) {
    struct {
        std::optional<uint32_t> id {};
        std::optional<std::string> vault_path {};
        bool multiline {};

    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");
    parser.add_argument(args.multiline, "--multiline", "-m").required(false).help("multiline");
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

    std::string secret_name {};
    std::string secret_content {};

    load_secret(secret_path, secret_key, secret_name, secret_content);

    std::string new_secret_content {};
    if (args.multiline) {
        std::cout << "Enter content and press Ctrl+D when finished" << std::endl;
        new_secret_content = read_text_until_eof();
    } else {
        secret_content = read_hidden_text_with_prompt("Enter password: ", true);
        if (read_hidden_text_with_prompt("Retype password: ", true) != secret_content) {
            std::cerr << "ERROR: passwords do not match" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    const auto sec_ret = save_secret_into_vault(vault_path, secret_name, new_secret_content, secret_key);
    if (sec_ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to save secret" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Ok, updated" << std::endl;
}
