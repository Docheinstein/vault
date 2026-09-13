#ifndef CLIUTILS_H
#define CLIUTILS_H

#include <optional>
#include <string>

#include "vault/types/securestring.h"

std::string read_line();
std::string read_line_with_prompt(const std::string& prompt);

std::optional<SecureString> read_hidden_line_secure();
std::optional<SecureString> read_line_secure();
std::optional<SecureString> read_multiline_secure();

std::optional<SecureString> read_hidden_line_with_prompt_secure(const std::string& prompt);
std::optional<SecureString> read_line_with_prompt_secure(const std::string& prompt);
std::optional<SecureString> read_multiline_with_prompt_secure(const std::string& prompt);

std::optional<uint64_t> read_number_with_prompt(const std::string& prompt);
bool read_yes_no_with_prompt(const std::string& prompt, bool default_yes = true);

class SecureCout {
public:
    void write(const unsigned char* buffer, size_t size);

    template <typename T>
    SecureCout& operator<<(const T& value) {
        std::cout << value;
        return *this;
    }

    SecureCout& operator<<(std::ostream& (*manip)(std::ostream&));
    SecureCout& operator<<(const SecureString& secret);
};

inline SecureCout secure_cout;

#endif // CLIUTILS_H
