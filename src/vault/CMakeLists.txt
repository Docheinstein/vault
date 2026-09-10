add_library(vault STATIC)

target_sources(vault PUBLIC
    vault/init.h
    vault/init.cpp
    vault/fs/fileio.h
    vault/fs/fileio.cpp
    vault/secure/securearray.h
    vault/secure/securestring.h
    vault/secure/securevector.h
    vault/utils/strings.h
    vault/utils/strings.cpp
    vault/vault/secret.h
    vault/vault/secret.cpp
    vault/vault/vault.h
    vault/vault/vault.cpp
)

target_include_directories(vault PUBLIC .)

target_link_libraries(vault PUBLIC libsodium_deps libgit2_deps)

add_dependencies(vault libsodium)
