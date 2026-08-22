#include "ui/prompt.h"

#include <iostream>
#include <termios.h>
#include <unistd.h>

#include "vault/utils/strings.h"

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
