add_library(vault STATIC)

target_sources(vault PUBLIC
    vault/common/retcodes.h
    vault/fs/fileio.h
    vault/fs/fileio.cpp
    vault/secure/securestring.h
    vault/secure/securestring.cpp
    vault/utils/strings.h
    vault/utils/strings.cpp
    vault/utils/vaults.h
    vault/utils/vaults.cpp
    vault/vault/secret.h
    vault/vault/secret.cpp
    vault/vault/vault.h
    vault/vault/vault.cpp
)

target_include_directories(vault PUBLIC .)

target_link_libraries(vault PUBLIC simdjson libsodium_deps libgit2_deps)

add_dependencies(vault libsodium)
