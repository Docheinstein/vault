#include "vault/init.h"

#include "sodium.h"

#ifdef ENABLE_GIT
#include "git2.h"
#endif

bool vault_init() {
#ifdef ENABLE_GIT
    return sodium_init() >= 0 && git_libgit2_init() > 0;
#else
    return sodium_init() >= 0;
#endif
}

bool vault_deinit() {
#ifdef ENABLE_GIT
    return git_libgit2_shutdown() >= 0;
#else
    return true;
#endif
}
