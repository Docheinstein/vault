#include "commands/add.h"

#include <cstring>
#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

#include "utils/env.h"
#include "utils/prompt.h"

#include <cstdlib>

int command_add(int argc, char** argv) {
    struct {
        bool multiline {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.multiline, "--multiline", "-m").required(false).help("multiline");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    const std::filesystem::path vault_master_file_path = (vault_path / ".vault");

    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        return EXIT_FAILURE;
    }

    const auto load_vault_result = load_vault(vault_master_file_path, *vault_password);
    if (!load_vault_result) {
        return EXIT_FAILURE;
    }

    Secret secret {};

    std::optional<SecureString> secret_name = read_line_with_prompt_secure("Name: ");
    if (!secret_name) {
        return EXIT_FAILURE;
    }

    std::optional<SecureString> secret_content;
    if (args.multiline) {
        secret_content = read_multiline_with_prompt_secure("Enter content and press Ctrl+D when finished\n");
        if (!secret_content) {
            return EXIT_FAILURE;
        }
    } else {
        secret_content = read_hidden_line_with_prompt_secure("Enter password: ");
        if (!secret_content) {
            return EXIT_FAILURE;
        }
        const auto secret_content_again = read_hidden_line_with_prompt_secure("Retype password: ");
        if (!secret_content_again) {
            return EXIT_FAILURE;
        }

        if (secret_content->size() != secret_content_again->size() ||
            sodium_memcmp(secret_content->data(), secret_content_again->data(), secret_content_again->size()) != 0) {
            std::cerr << "ERROR: passwords do not match" << std::endl;
            return EXIT_FAILURE;
        }
    }

    secret.name = std::move(*secret_name);
    secret.content = std::move(*secret_content);

    const auto save_secret_result = save_secret(vault_path, *load_vault_result, secret, false);

    if (!save_secret_result) {
        std::cerr << "ERROR: failed to save secret" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
