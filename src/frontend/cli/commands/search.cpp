#include "commands/search.h"

#include <iostream>
#include <optional>
#include <string>
#include <unistd.h>

#include "args/args.h"

#include "vault/utils/strings.h"
#include "vault/vault/vault.h"

#include "utils/colors.h"
#include "utils/env.h"
#include "utils/prompt.h"

#include "utils/vaults.h"

namespace {
const char* search_string_case_insensitive(const char* haystack, size_t haystack_len, const char* needle,
                                           size_t needle_len) {
    size_t haystack_cursor = 0;
    size_t needle_cursor = 0;

    while (haystack_cursor < haystack_len && needle_cursor < needle_len) {
        if (tolower(haystack[haystack_cursor]) == tolower(needle[needle_cursor])) {
            ++needle_cursor;
        } else {
            needle_cursor = 0;
        }
        ++haystack_cursor;
    }

    if (haystack_cursor < haystack_len) {
        return &haystack[haystack_cursor - needle_len];
    }

    return nullptr;
}

void print_highlight_case_insensitive(const char* haystack, const size_t haystack_len, const char* needle,
                                      const size_t needle_len) {
    const char* remaining_haystack = haystack;
    size_t remaining_len = haystack_len;

    const char* substr_match = nullptr;
    do {
        substr_match = search_string_case_insensitive(remaining_haystack, remaining_len, needle, needle_len);

        if (!substr_match) {
            write(STDOUT_FILENO, remaining_haystack, remaining_len);
        } else {
            write(STDOUT_FILENO, remaining_haystack, substr_match - remaining_haystack);

            std::cout << BOLD << RED << std::flush;

            write(STDOUT_FILENO, substr_match, needle_len);

            std::cout << RESET << std::flush;

            remaining_len = remaining_len - (substr_match - remaining_haystack) - needle_len;
            remaining_haystack = substr_match + needle_len;
        }
    } while (substr_match);
}

} // namespace

int command_search(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_path {};
        std::optional<std::string> search_pattern {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.search_pattern, "pattern").required(false).help("search pattern");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vaults_path();

    const auto search_pattern = args.search_pattern ? *args.search_pattern : read_line_with_prompt("Search pattern: ");
    const std::string search_pattern_lower = string_to_lower(search_pattern);

    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        std::cerr << "ERROR: failed to load vault pw" << std::endl;
        return EXIT_FAILURE;
    }

    const std::filesystem::path vault_master_file_path = (vault_path / ".vault");

    const auto load_vault_result = load_vault(vault_master_file_path, *vault_password);
    if (!load_vault_result) {
        std::cerr << "ERROR: failed to load vault" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> secrets_path = get_all_secrets(vault_path);

    bool first_secret = true;

    for (uint32_t i = 0; i < secrets_path.size(); i++) {
        const auto& secret_path = secrets_path[i];

        auto secret = load_secret(secret_path, *load_vault_result);
        if (!secret) {
            std::cerr << "ERROR: failed to load secret" << std::endl;
            return EXIT_FAILURE;
        }

        bool match_name =
            search_string_case_insensitive(reinterpret_cast<const char*>(secret->name.data()), secret->name.size(),
                                           search_pattern.c_str(), search_pattern.size());

        bool match_content =
            search_string_case_insensitive(reinterpret_cast<const char*>(secret->content.data()),
                                           secret->content.size(), search_pattern.c_str(), search_pattern.size());

        if (match_name || match_content) {
            if (!first_secret) {
                std::cout << "\n";
            }

            first_secret = false;

            std::cout << i << ". " << std::flush;

            print_highlight_case_insensitive(reinterpret_cast<const char*>(secret->name.data()), secret->name.size(),
                                             search_pattern.c_str(), search_pattern.size());
            write(STDOUT_FILENO, "\n", 1);
            print_highlight_case_insensitive(reinterpret_cast<const char*>(secret->content.data()),
                                             secret->content.size(), search_pattern.c_str(), search_pattern.size());
            write(STDOUT_FILENO, "\n", 1);
        }
    }

    return EXIT_SUCCESS;
}
