#ifndef TUI_PROMPTUTILS_H
#define TUI_PROMPTUTILS_H

#include <optional>
#include <string>

#include "vault/secure/securestring.h"

uint32_t read_number_with_prompt(const std::string& prompt);

std::optional<SecureString> read_hidden_line_secure();
std::optional<SecureString> read_line_secure();
std::optional<SecureString> read_multiline_secure();

std::optional<SecureString> read_hidden_line_with_prompt_secure(const std::string& prompt);
std::optional<SecureString> read_line_with_prompt_secure(const std::string& prompt);
std::optional<SecureString> read_multiline_with_prompt_secure(const std::string& prompt);

bool read_yes_no_with_prompt(const std::string& prompt, bool default_yes = true);

void secure_write(const SecureString& text, bool newline = true);

std::string read_line();

std::string read_line_with_prompt(const std::string& prompt);

#endif // TUI_PROMPTUTILS_H
