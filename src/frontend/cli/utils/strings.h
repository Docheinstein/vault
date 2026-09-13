#ifndef STRINGSUTILS_H
#define STRINGSUTILS_H

#include <cstdint>
#include <optional>
#include <string>

std::optional<uint64_t> strtou(const std::string& s, uint8_t base = 10);

#endif
