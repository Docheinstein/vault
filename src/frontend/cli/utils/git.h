#ifndef GITUTILS_H
#define GITUTILS_H

#include <filesystem>

bool has_git_repository(const std::filesystem::path& path);

#endif // GITUTILS_H
