#include "utils/prompt.h"

#include <iostream>

#include "vault/utils/strings.h"

#include "ui/prompt.h"

#include "commands/exitcodes.h"

std::string read_text_with_prompt(const std::string& prompt) {
    std::string text;
    while (text.empty()) {
        std::cout << prompt;
        text = get_text();
    }
    return text;
}

std::string read_hidden_text_with_prompt(const std::string& prompt) {
    std::string text;
    while (text.empty()) {
        std::cout << prompt;
        text = get_hidden_text();
    }
    return text;
}

uint32_t read_number_with_prompt(const std::string& prompt) {
    const std::optional<uint64_t> id = strtou(read_text_with_prompt(prompt));
    if (!id) {
        exit(EXIT_EXECUTION_FAILED);
    }

    return static_cast<uint32_t>(*id);
}

std::string read_vault_name() {
    return read_text_with_prompt("Vault: ");
}

std::string read_password() {
    return read_hidden_text_with_prompt("Password: ");
}

uint32_t read_id() {
    return read_number_with_prompt("ID: ");
}

bool read_yes_no_answer(bool default_answer) {
    const char default_char = default_answer ? 'n' : 'y';
    const std::string hidden = get_text();
    return hidden.size() == 1 && std::tolower(hidden[0]) == default_char;
}