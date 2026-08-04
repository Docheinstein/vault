#ifndef TUI_DISPLAYUTILS_H
#define TUI_DISPLAYUTILS_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "vault/vault/vault.h"

struct PrintableEntry {
    uint32_t id {};
    std::vector<std::string> values {};
};

std::vector<PrintableEntry> build_entries(const Vault& vault, std::optional<uint32_t> sort_field_index = std::nullopt);

void print_fields(const Vault& vault);

template <std::ranges::input_range EntriesRange>
void print_entries(
    const Vault& vault, EntriesRange&& entries,
    const std::function<void(std::string& field)>& apply_to_value = [](std::string& _) {
    }) {
    for (const auto& entry : entries) {
        std::cout << entry.id << " | ";

        for (uint32_t value_index = 0; value_index < entry.values.size(); ++value_index) {
            auto value = entry.values[value_index];
            apply_to_value(value);

            std::cout << value;

            if (value_index < entry.values.size() - 1) {
                std::cout << " | ";
            }
        }
        std::cout << std::endl;
    }
}

std::string get_entry_field_value(const Vault& vault, const Vault::Entry& entry, uint32_t field_index);

#endif // TUI_DISPLAYUTILS_H
