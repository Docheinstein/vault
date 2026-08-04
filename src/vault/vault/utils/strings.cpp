#include "vault/utils/strings.h"

#include <sstream>

bool string_compare_case_insensitive(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (uint32_t i = 0; i < lhs.size(); i++) {
        if (std::tolower(lhs[i]) != std::tolower(rhs[i])) {
            return false;
        }
    }

    return true;
}

std::string string_to_lower(std::string_view str) {
    std::stringstream out;

    for (const char i : str) {
        out << static_cast<char>(std::tolower(i));
    }

    return out.str();
}

std::optional<uint64_t> strtou(const std::string& s, uint8_t base) {
    const char* const cstr = s.c_str();
    char* endptr {};
    errno = 0;

    const auto val = std::strtoull(cstr, &endptr, base);

    if (errno || endptr == cstr || *endptr != '\0') {
        return std::nullopt;
    }

    return val;
}

bool string_contains(std::string_view lhs, std::string_view rhs) {
    return lhs.find(rhs) != std::string::npos;
}
