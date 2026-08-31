#include "utils/prompt.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "vault/utils/strings.h"

#include "commands/exitcodes.h"

std::string get_hidden_text() {
    // Cache current terminal settings.
    termios oldt {};
    tcgetattr(STDIN_FILENO, &oldt);

    // Disable ECHO.
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::string text {};
    std::getline(std::cin, text);

    // Restore previous terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;

    return text;
}

std::string get_text() {
    std::string text {};
    getline(std::cin, text);
    return text;
}

uint64_t get_number() {
    std::string text {};
    std::optional<uint64_t> number {};
    do {
        text = get_text();
        number = strtou(text);
    } while (!number);

    return *number;
}

std::string read_text_with_prompt(const std::string& prompt) {
    std::string text;
    while (text.empty()) {
        std::cout << prompt;
        text = get_text();
    }
    return text;
}

std::string read_hidden_text_with_prompt(const std::string& prompt, bool allow_empty) {
    std::string text;
    do {
        std::cout << prompt;
        text = get_hidden_text();
    } while (!allow_empty && text.empty());

    return text;
}

uint32_t read_number_with_prompt(const std::string& prompt) {
    const std::optional<uint64_t> id = strtou(read_text_with_prompt(prompt));
    if (!id) {
        exit(EXIT_EXECUTION_FAILED);
    }

    return static_cast<uint32_t>(*id);
}

std::string read_text_until_eof() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}
