#ifndef CLI_CMEXITCODES_H
#define CLI_CMEXITCODES_H

#include <cstdint>

constexpr uint8_t EXIT_UNKNOWN_COMMAND = 2;
constexpr uint8_t EXIT_EXECUTION_FAILED = 3;
constexpr uint8_t EXIT_IO_ERROR = 4;
constexpr uint8_t EXIT_ENCRYPTION_ERROR = 5;
constexpr uint8_t EXIT_EDITOR_UNSET = 6;
constexpr uint8_t EXIT_VAULT_NOT_INITIALIZED = 7;

#endif // CLI_CMEXITCODES_H
