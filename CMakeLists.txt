cmake_minimum_required(VERSION 3.24)
project(vault)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(DEBUG_STORE_PLAINTEXT "Store/Load plain text instead of encrypted data" OFF)

if (DEBUG_STORE_PLAINTEXT)
    add_compile_definitions(DEBUG_STORE_PLAINTEXT)
endif()

add_subdirectory(third_party)
add_subdirectory(src)

target_include_directories(vault PRIVATE .)
