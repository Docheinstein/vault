#ifndef VAULTSUTILS_H
#define VAULTSUTILS_H

#include <filesystem>

bool is_secret_file(const std::filesystem::path& path);

#endif // VAULTSUTILS_H
