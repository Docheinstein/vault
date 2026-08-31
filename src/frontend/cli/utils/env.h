#ifndef TUI_ENVUTILS_H
#define TUI_ENVUTILS_H

#include <filesystem>

const char* get_default_home_path();
std::filesystem::path get_default_vaults_path();

#endif // TUI_ENVUTILS_H
