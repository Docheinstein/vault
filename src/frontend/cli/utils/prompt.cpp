#include "utils/prompt.h"

#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "vault/utils/strings.h"

#include "commands/exitcodes.h"

uint32_t read_number_with_prompt(const std::string& prompt) {
    const std::optional<uint64_t> id = strtou(read_line_with_prompt(prompt));
    if (!id) {
        exit(EXIT_EXECUTION_FAILED);
    }

    return static_cast<uint32_t>(*id);
}
namespace {
constexpr size_t STDIN_READ_CHUNK_SIZE = 256;
}

std::optional<SecureString> secure_read_hidden_line() {
    // Cache current terminal settings.
    termios oldt {};
    tcgetattr(STDIN_FILENO, &oldt);

    // Disable ECHO.
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::optional<SecureString> line = secure_read_line();

    // Restore previous terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;

    return line;
}

std::optional<SecureString> secure_read_line() {
    SecureString text {};

    while (true) {
        unsigned char* buf = text.prepare_append(STDIN_READ_CHUNK_SIZE);
        const ssize_t ret = read(STDIN_FILENO, buf, STDIN_READ_CHUNK_SIZE);
        if (ret < 0) {
            return std::nullopt;
        }

        if (ret == 0) {
            // EOF
            break;
        }
        text.commit_append(ret);

        if (text.data()[text.size() - 1] == '\n') {
            text.resize(text.size() - 1);
            break;
        }

        // TODO: maybe quit anyway for sending only one ctrl+d?
        if (static_cast<size_t>(ret) < STDIN_READ_CHUNK_SIZE) {
            break;
        }
    }
    return text;
}

std::optional<SecureString> secure_read_multiline() {
    SecureString text {};

    while (true) {
        unsigned char* buf = text.prepare_append(STDIN_READ_CHUNK_SIZE);
        const ssize_t ret = read(STDIN_FILENO, buf, STDIN_READ_CHUNK_SIZE);
        // std::cout << "ret: " << ret << std::endl;
        if (ret < 0) {
            return std::nullopt;
        }

        if (ret == 0) {
            // EOF
            break;
        }
        text.commit_append(ret);
    }

    if (text.data()[text.size() - 1] == '\n') {
        text.resize(text.size() - 1);
    }

    return text;
}

std::optional<SecureString> secure_read_hidden_line_with_prompt(const std::string& prompt) {
    std::cout << prompt << std::flush;
    return secure_read_hidden_line();
}

std::optional<SecureString> secure_read_line_with_prompt(const std::string& prompt) {
    std::cout << prompt << std::flush;
    return secure_read_line();
}

std::optional<SecureString> secure_read_multiline_with_prompt(const std::string& prompt) {
    std::cout << prompt << std::flush;
    return secure_read_multiline();
}

bool read_yes_no_with_prompt(const std::string& prompt, bool default_yes) {
    std::cout << prompt << std::flush;
    std::string input;
    std::getline(std::cin, input);
    return input.empty() ? default_yes : tolower(input[0]) == 'y';
}

void secure_write(const SecureString& string, const bool newline) {
    write(STDOUT_FILENO, string.data(), string.size());
    if (newline) {
        write(STDOUT_FILENO, "\n", 1);
    }
}

std::string read_line() {
    std::string text {};
    getline(std::cin, text);
    return text;
}

std::string read_line_with_prompt(const std::string& prompt) {
    std::string text;
    while (text.empty()) {
        std::cout << prompt;
        text = read_line();
    }
    return text;
}
