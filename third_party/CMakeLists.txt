# args
add_subdirectory(args)

# simdjson
add_subdirectory(simdjson)

# libsodium
include(ExternalProject)

set(LIBSODIUM_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/libsodium)
set(LIBSODIUM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libsodium)

ExternalProject_Add(libsodium
        USES_TERMINAL_DOWNLOAD true
        USES_TERMINAL_UPDATE true
        USES_TERMINAL_PATCH true
        USES_TERMINAL_CONFIGURE true
        USES_TERMINAL_BUILD true
        USES_TERMINAL_INSTALL true
        USES_TERMINAL_TEST true
        PREFIX ${LIBSODIUM_INSTALL_DIR}
        SOURCE_DIR ${LIBSODIUM_SOURCE_DIR}
        CONFIGURE_COMMAND ${LIBSODIUM_SOURCE_DIR}/configure --prefix ${LIBSODIUM_INSTALL_DIR}
        BUILD_COMMAND make -j14
        INSTALL_COMMAND make install
        BUILD_BYPRODUCTS ${LIBSODIUM_INSTALL_DIR}/lib/libsodium.a
)

add_library(libsodium_deps INTERFACE)

target_include_directories(libsodium_deps INTERFACE ${LIBSODIUM_INSTALL_DIR}/include)
target_link_libraries(libsodium_deps INTERFACE ${LIBSODIUM_INSTALL_DIR}/lib/libsodium.a)

# libgit2
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Shared Library (OFF for Static)" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build Tests using the Clar suite" FORCE)
set(BUILD_CLI OFF CACHE BOOL  "Build the command-line interface" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL   "Build library usage example apps" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL   "Build the fuzz targets" FORCE)

set(USE_HTTPS OpenSSL)
set(USE_BUNDLED_ZLIB ON)

find_package(OpenSSL REQUIRED)
add_subdirectory(libgit2)

add_library(libgit2_deps INTERFACE)
target_link_libraries(libgit2_deps INTERFACE
    zlib util libgit2 ntlmclient llhttp pcre2 reftable xdiff OpenSSL::SSL)

