#include "vault/init.h"

#include "sodium.h"

bool vault_init() {
    return sodium_init() >= 0;
}
