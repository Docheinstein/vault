#include "utils/prompt.h"

#include <iostream>

#include "vault/utils/strings.h"

#include "ui/prompt.h"

#include "commands/exitcodes.h"

std::string read_with_prompt(const std::string& prompt) {
    std::string vault_name;
    while (vault_name.empty()) {
        std::cout << prompt;
        vault_name = get_text();
    }
    return vault_name;
}

std::string read_vault_name() {
    return read_with_prompt("Vault: ");
}

uint32_t read_number_with_prompt(const std::string& prompt) {
    const std::optional<uint64_t> id = strtou(read_with_prompt(prompt));
    if (!id) {
        exit(EXIT_EXECUTION_FAILED);
    }

    return static_cast<uint32_t>(*id);
}

uint32_t read_id() {
    return read_number_with_prompt("ID: ");
}

bool read_yes_no_answer(bool default_answer) {
    const char default_char = default_answer ? 'n' : 'y';
    const std::string hidden = get_text();
    return hidden.size() == 1 && std::tolower(hidden[0]) == default_char;
}