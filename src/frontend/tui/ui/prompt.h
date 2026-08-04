#ifndef TUI_PROMPT_H
#define TUI_PROMPT_H

#include <cstdint>
#include <string>

std::string get_password();
std::string get_text();

uint64_t get_number();

#endif // TUI_PROMPT_H
