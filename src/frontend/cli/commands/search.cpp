#include "commands/search.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

#include "utils/cli.h"
#include "utils/colors.h"
#include "utils/vault.h"

#include "result.h"

namespace {
const unsigned char* search_string_case_insensitive(const unsigned char* haystack, const size_t haystack_len,
                                                    const unsigned char* needle, const size_t needle_len) {
    if (needle_len > haystack_len) {
        return nullptr;
    }

    for (size_t haystack_cursor_start = 0; haystack_cursor_start <= haystack_len - needle_len;
         ++haystack_cursor_start) {
        size_t haystack_cursor = haystack_cursor_start;
        size_t needle_cursor = 0;
        while (needle_cursor < needle_len && tolower(haystack[haystack_cursor]) == tolower(needle[needle_cursor])) {
            ++haystack_cursor;
            ++needle_cursor;
        }

        if (needle_cursor == needle_len) {
            return &haystack[haystack_cursor_start];
        }
    }

    return nullptr;
}

void print_highlight_match_case_insensitive(const unsigned char* haystack, const size_t haystack_len,
                                            const unsigned char* needle, const size_t needle_len) {
    const unsigned char* remaining_haystack = haystack;
    size_t remaining_len = haystack_len;

    const unsigned char* substr_match = nullptr;
    do {
        substr_match = search_string_case_insensitive(remaining_haystack, remaining_len, needle, needle_len);

        if (!substr_match) {
            secure_cout.write(remaining_haystack, remaining_len);
        } else {
            secure_cout.write(remaining_haystack, substr_match - remaining_haystack);

            secure_cout << BOLD << RED;
            secure_cout.write(substr_match, needle_len);
            secure_cout << RESET;

            remaining_len = remaining_len - (substr_match - remaining_haystack) - needle_len;
            remaining_haystack = substr_match + needle_len;
        }
    } while (substr_match);
}

} // namespace

VaultCommandResult command_search(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_path {};
        std::optional<std::string> search_pattern {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.search_pattern, "pattern").required(false).help("search pattern");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        return VAULT_COMMAND_ARGS_PARSE_ERROR;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vault_path();

    const std::filesystem::path vault_master_file_path = get_vault_master_file_path(vault_path);

    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        return VAULT_STDIN_ERROR;
    }

    const auto vault_key = load_vault(vault_master_file_path, *vault_password);
    if (!vault_key) {
        return VAULT_VAULT_LOAD_ERROR;
    }

    const auto search_pattern = args.search_pattern ? *args.search_pattern : read_line_with_prompt("Search pattern: ");
    const auto* const search_pattern_data = reinterpret_cast<const unsigned char*>(search_pattern.c_str());

    if (search_pattern.empty()) {
        return VAULT_INVALID_SEARCH_PATTERN_ERROR;
    }

    const std::vector<std::string> secrets_paths = get_vault_secrets(vault_path);

    bool first_matching_secret = true;

    for (uint32_t i = 0; i < secrets_paths.size(); i++) {
        const auto& secret_path = secrets_paths[i];

        const auto secret = load_secret(secret_path, *vault_key);
        if (!secret) {
            // Silently skip corrupted entries.
            continue;
        }

        if (search_string_case_insensitive(secret->name.data(), secret->name.size(), search_pattern_data,
                                           search_pattern.size()) ||
            search_string_case_insensitive(secret->content.data(), secret->content.size(), search_pattern_data,
                                           search_pattern.size())) {
            if (!first_matching_secret) {
                secure_cout << std::endl;
            }

            first_matching_secret = false;

            secure_cout << i << ". " << std::flush;

            print_highlight_match_case_insensitive(secret->name.data(), secret->name.size(), search_pattern_data,
                                                   search_pattern.size());
            secure_cout << std::endl;

            print_highlight_match_case_insensitive(secret->content.data(), secret->content.size(), search_pattern_data,
                                                   search_pattern.size());
            secure_cout << std::endl;
        }
    }

    return VAULT_SUCCESS;
}
