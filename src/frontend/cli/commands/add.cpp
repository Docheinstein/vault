#include "commands/add.h"

#include <cstring>
#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/utils/strings.h"
#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

#include "utils/env.h"
#include "utils/prompt.h"

#include "commands/exitcodes.h"

void command_add(int argc, char** argv) {

    struct {
        std::optional<std::string> secret_name {};
        bool multiline {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.secret_name, "name").required(false).help("secret name");
    parser.add_argument(args.multiline, "--multiline", "-m").required(false).help("multiline");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    const std::filesystem::path vault_master_file_path = (vault_path / ".vault");

    const std::string vault_password = read_hidden_text_with_prompt("Enter vault password: ");

    unsigned char secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    const bool ret = load_vault((vault_path / ".vault").string(), vault_password, secret_key);

    if (ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to open vault" << std::endl;
        exit(EXIT_FAILURE);
    }

    const std::string secret_name = args.secret_name.has_value() ? *args.secret_name : read_text_with_prompt("Name: ");

    std::string secret_content {};
    if (args.multiline) {
        std::cout << "Enter content and press Ctrl+D when finished" << std::endl;
        secret_content = read_text_until_eof();
        rtrim(secret_content);
    } else {
        secret_content = read_hidden_text_with_prompt("Enter password: ", true);
        if (read_hidden_text_with_prompt("Retype password: ", true) != secret_content) {
            std::cerr << "ERROR: passwords do not match" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // std::filesystem::path secret_path = (vault_path / secret_name_obscure).string();

    const auto sec_ret = save_secret_into_vault(vault_path, secret_name, secret_content, secret_key, false);
    if (sec_ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to save secret" << std::endl;
        exit(EXIT_FAILURE);
    }
}
