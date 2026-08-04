#ifndef STRINGSUTILS_H
#define STRINGSUTILS_H

#include <filesystem>

#include <optional>

#include "vault/fs/fileio.h"

bool string_compare_case_insensitive(std::string_view lhs, std::string_view rhs);
bool string_contains(std::string_view lhs, std::string_view rhs);

std::string string_to_lower(std::string_view str);

std::optional<uint64_t> strtou(const std::string& str, uint8_t base = 10);

#endif
