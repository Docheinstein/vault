#ifndef TUI_PROMPTUTILS_H
#define TUI_PROMPTUTILS_H

#include <cstdint>
#include <string>

std::string get_hidden_text();
std::string get_text();

uint64_t get_number();

std::string read_text_with_prompt(const std::string& prompt);
std::string read_hidden_text_with_prompt(const std::string& prompt, bool allow_empty = false);
uint32_t read_number_with_prompt(const std::string& prompt);

std::string read_text_until_eof();

#endif // TUI_PROMPTUTILS_H
