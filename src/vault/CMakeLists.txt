add_library(vault STATIC)

target_sources(vault PUBLIC
    vault/init.h
    vault/init.cpp
    vault/fs/fileio.h
    vault/fs/fileio.cpp
    vault/types/securearray.h
    vault/types/securestring.h
    vault/types/securevector.h
    vault/vault/secret.h
    vault/vault/secret.cpp
    vault/vault/vault.h
    vault/vault/vault.cpp
)

target_include_directories(vault PUBLIC .)

target_link_libraries(vault PUBLIC libsodium_deps libgit2_deps)

add_dependencies(vault libsodium)
