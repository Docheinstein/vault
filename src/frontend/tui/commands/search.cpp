#include "commands/search.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>

#include "args/args.h"

#include "vault/utils/strings.h"

#include "ui/colors.h"

#include "utils/display.h"
#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_search(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_name {};
        std::optional<std::string> search_pattern {};
        std::optional<std::string> sort_by {};
        std::optional<std::string> vaults_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_name, "name").required(false).help("vault name");
    parser.add_argument(args.search_pattern, "pattern").required(false).help("search pattern");
    parser.add_argument(args.sort_by, "--sort", "-s").required(false).help("sort by field");
    parser.add_argument(args.vaults_path, "--path").required(false).help("vaults path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::string vault_name = args.vault_name.has_value() ? *args.vault_name : read_vault_name();
    const std::filesystem::path vaults_path =
        args.vaults_path.has_value() ? std::filesystem::path {*args.vaults_path} : get_default_vaults_path();
    const std::filesystem::path vault_path = vaults_path / vault_name;

    const std::string search_pattern =
        args.search_pattern ? *args.search_pattern : read_text_with_prompt("Search pattern: ");
    const std::string search_pattern_lower = string_to_lower(search_pattern);

    const std::string password = read_password();

    Vault vault {};
    load_vault(vault, vault_path, password);

    print_fields(vault);

    const std::optional<uint32_t> sort_field_index =
        args.sort_by ? get_field_index(vault, *args.sort_by) : std::nullopt;
    const std::vector<PrintableEntry> sorted_entries = build_entries(vault, sort_field_index);

    // Keep only entries having at least a field matching the pattern.
    auto filtered_entries =
        std::ranges::filter_view(sorted_entries, [&search_pattern_lower](const PrintableEntry& entry) {
            return std::ranges::any_of(entry.values, [&search_pattern_lower](const std::string& field) {
                return string_contains(string_to_lower(field), search_pattern_lower);
            });
        });

    print_entries(vault, filtered_entries, [&search_pattern_lower](std::string& field) {
        // Highlight in red the matching parts of the fields.
        std::string out;
        do {
            if (const auto pos = string_to_lower(field).find(search_pattern_lower); pos != std::string::npos) {
                out += field.substr(0, pos);
                out += red(field.substr(pos, search_pattern_lower.size()));
                field = field.substr(pos + search_pattern_lower.size());
            } else {
                out += field;
                break;
            }

        } while (!field.empty());

        field = out;
    });
}
