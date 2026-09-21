#include "utils/cli.h"

#include <iostream>
#include <termios.h>
#include <unistd.h>

#include "utils/strings.h"

std::string read_line() {
    std::string text {};
    getline(std::cin, text);
    return text;
}

std::string read_line_with_prompt(const std::string& prompt) {
    std::cout << prompt;
    return read_line();
}

std::optional<SecureString> read_hidden_line_secure() {
    // Cache current terminal settings.
    termios oldt {};
    tcgetattr(STDIN_FILENO, &oldt);

    // Disable ECHO.
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::optional<SecureString> line = read_line_secure();

    // Restore previous terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;

    return line;
}

std::optional<SecureString> read_line_secure() {
    SecureString text {};

    while (true) {
        unsigned char c {};

        // Read the next byte.
        const ssize_t ret = read(STDIN_FILENO, &c, 1);

        if (ret < 0) {
            // Read error.
            return std::nullopt;
        }

        if (ret == 0) {
            // EOF.
            break;
        }

        // Stop as soon as new line is found.
        if (c == '\n') {
            break;
        }

        text.append(&c, 1);
    }

    return text;
}

std::optional<SecureString> read_multiline_secure() {
    static constexpr size_t READ_CHUNK_SIZE = 256;

    SecureString text {};

    while (true) {
        // Reserve space for the next read.
        text.reserve(text.size() + READ_CHUNK_SIZE);

        unsigned char* const buffer = text.data() + text.size();

        // Read the next 256 bytes.
        const ssize_t ret = read(STDIN_FILENO, buffer, READ_CHUNK_SIZE);

        if (ret < 0) {
            // Read error.
            return std::nullopt;
        }

        if (ret == 0) {
            // EOF.
            break;
        }

        // Actually update the string size.
        text.resize(text.size() + ret);
    }

    // Eventually trim last new line.
    if (text.size() > 0 && text.data()[text.size() - 1] == '\n') {
        text.resize(text.size() - 1);
    }

    return text;
}

std::optional<SecureString> read_hidden_line_with_prompt_secure(const std::string& prompt) {
    std::cout << prompt << std::flush;
    return read_hidden_line_secure();
}

std::optional<SecureString> read_line_with_prompt_secure(const std::string& prompt) {
    std::cout << prompt << std::flush;
    return read_line_secure();
}

std::optional<SecureString> read_multiline_with_prompt_secure(const std::string& prompt) {
    std::cout << prompt << std::flush;
    return read_multiline_secure();
}

std::optional<uint64_t> read_number_with_prompt(const std::string& prompt) {
    return strtou(read_line_with_prompt(prompt));
}

bool read_yes_no_with_prompt(const std::string& prompt, bool default_yes) {
    std::cout << prompt << std::flush;
    std::string input;
    std::getline(std::cin, input);
    return input.empty() ? default_yes : tolower(input[0]) == 'y';
}

SecureCout& SecureCout::operator<<(std::ostream& (*manip)(std::ostream&)) {
    std::cout << manip;
    return *this;
}

SecureCout& SecureCout::operator<<(const SecureString& secret) {
    write(secret.data(), secret.size());
    return *this;
}

void SecureCout::write(const unsigned char* const buffer, size_t size) {
    std::cout.flush();
    ::write(STDOUT_FILENO, buffer, size);
}
