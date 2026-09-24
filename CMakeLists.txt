cmake_minimum_required(VERSION 3.24)
project(vault)

set(CMAKE_CXX_STANDARD 23)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(ENABLE_GIT "Build with libgit to support sync of secrets with remote repository" ON)

if (ENABLE_GIT)
    add_compile_definitions(ENABLE_GIT)
endif()

add_subdirectory(third_party)
add_subdirectory(src)

target_include_directories(vault PRIVATE .)
