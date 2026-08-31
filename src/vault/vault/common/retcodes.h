#ifndef ERRCODES_H
#define ERRCODES_H

#include <cstdint>

using VaultReturnCode = uint8_t;

constexpr VaultReturnCode VAULT_SUCCESS = 0;
constexpr VaultReturnCode VAULT_GENERIC_ERROR = 1;

#endif // ERRCODES_H
