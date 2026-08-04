#include "utils/display.h"

#include "commands/exitcodes.h"
#include "vaults.h"

std::vector<PrintableEntry> build_entries(const Vault& vault, std::optional<uint32_t> sort_field_index) {
    std::vector<PrintableEntry> sorted_entries {};
    sorted_entries.resize(vault.entries.size());

    // Take all the fields values.
    uint32_t id = 0;
    std::transform(vault.entries.begin(), vault.entries.end(), sorted_entries.begin(),
                   [&id](const Vault::Entry& entry) {
                       PrintableEntry printable_entry {};
                       printable_entry.values = entry.values;
                       printable_entry.id = ++id;
                       return printable_entry;
                   });

    // Eventually sort the entries.
    if (sort_field_index) {
        std::sort(sorted_entries.begin(), sorted_entries.end(),
                  [sort_field_index = *sort_field_index](const PrintableEntry& lhs, const PrintableEntry& rhs) {
                      return lhs.values[sort_field_index] < rhs.values[sort_field_index];
                  });
    }

    return sorted_entries;
}

void print_fields(const Vault& vault) {
    std::cout << " ID | ";

    for (uint32_t field_index = 0; field_index < vault.fields.size(); ++field_index) {
        std::cout << vault.fields[field_index].name;
        if (field_index < vault.fields.size() - 1) {
            std::cout << " | ";
        }
    }

    std::cout << std::endl;

    std::cout << "-----------" << std::endl;
}

std::string get_entry_field_value(const Vault& vault, const Vault::Entry& entry, uint32_t field_index) {
    check_field_index_for_vault(vault, field_index);
    check_field_index_for_entry(vault, entry, field_index);

    if (field_index >= entry.values.size()) {
        std::cerr << "ERROR: entry has no value for field '" << vault.fields[field_index].name << "'" << std::endl;
        exit(EXIT_EXECUTION_FAILED);
    }

    if (vault.fields[field_index].hidden) {
        // Hidden field: obfuscate content with *.
        std::string hidden(entry.values[field_index].size(), '*');
        return hidden;
    }

    return entry.values[field_index];
}
