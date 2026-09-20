#ifndef CMDRESULT_H
#define CMDRESULT_H

#include <string>

#include "retcodes.h"

struct VaultCommandResult {
    VaultCommandResult(const int code) :
        code(code) {
    }

    VaultCommandResult(const int code, const char* message) :
        code {code},
        message {message} {
    }
    VaultCommandResult(const int code, std::string&& message) :
        code {code},
        message {std::move(message)} {
    }

    int code {VAULT_SUCCESS};
    std::string message {};
};

#endif // CMDRESULT_H
