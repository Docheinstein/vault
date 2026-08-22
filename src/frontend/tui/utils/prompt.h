#ifndef TUI_PROMPTUTILS_H
#define TUI_PROMPTUTILS_H

#include <cstdint>
#include <string>

std::string read_text_with_prompt(const std::string& prompt);
std::string read_hidden_text_with_prompt(const std::string& prompt);
uint32_t read_number_with_prompt(const std::string& prompt);

std::string read_vault_name();
std::string read_password();
uint32_t read_id();

bool read_yes_no_answer(bool default_answer);

#endif // TUI_PROMPTUTILS_H
