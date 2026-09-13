#include "utils/strings.h"

#include <sstream>

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
