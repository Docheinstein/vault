#ifndef TUI_COLORS_H
#define TUI_COLORS_H

#include <string>

#define ATTR(c) "\033[" #c "m"
#define RESET() ATTR(0)
#define ATTRIBUTIZE(c, t) ATTR(c) + t + RESET()

inline std::string red(const std::string& text) {
    return ATTRIBUTIZE(31, text);
}

inline std::string cyan(const std::string& text) {
    return ATTRIBUTIZE(34, text);
}

inline std::string bold(const std::string& text) {
    return ATTRIBUTIZE(1, text);
}

#endif // TUI_COLORS_H
