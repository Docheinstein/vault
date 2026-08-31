#include "commands/search.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/utils/strings.h"
#include "vault/vault/vault.h"

#include "utils/colors.h"
#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_search(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_path {};
        std::optional<std::string> search_pattern {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.search_pattern, "pattern").required(false).help("search pattern");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    const std::string search_pattern =
        args.search_pattern ? *args.search_pattern : read_text_with_prompt("Search pattern: ");
    const std::string search_pattern_lower = string_to_lower(search_pattern);

    // Check vault key.
    const std::string vault_password = read_hidden_text_with_prompt("Enter vault password: ");

    unsigned char secret_key[ENCRYPTION_SECRET_KEY_SIZE];
    const bool ret = load_vault((vault_path / ".vault").string(), vault_password, secret_key);

    if (ret != VAULT_SUCCESS) {
        std::cerr << "ERROR: failed to open vault" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    const auto highlight_matching = [](std::string str, const std::string& pattern) {
        std::string out;
        do {
            if (const auto pos = string_to_lower(str).find(pattern); pos != std::string::npos) {
                out += str.substr(0, pos);
                out += red(str.substr(pos, pattern.size()));
                str = str.substr(pos + pattern.size());
            } else {
                out += str;
                break;
            }

        } while (!str.empty());

        return out;
    };

    for (uint32_t i = 0; i < secrets_path.size(); i++) {
        const auto& secret_path = secrets_path[i];

        std::string secret_name {};
        std::string secret_content {};

        load_secret(secret_path, secret_key, secret_name, secret_content);

        if (string_to_lower(secret_name).find(search_pattern_lower) != std::string::npos ||
            string_to_lower(secret_content).find(search_pattern_lower) != std::string::npos) {
            std::cout << i << ". " << bold(highlight_matching(secret_name, search_pattern)) << "\n"
                      << highlight_matching(secret_content, search_pattern) << "\n"
                      << std::endl;
        }
    }
}
