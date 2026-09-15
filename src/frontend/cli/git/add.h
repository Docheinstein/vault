#ifndef GITADD_H
#define GITADD_H

#include <filesystem>

int vault_git_add(const std::filesystem::path& repo_path, const std::filesystem::path& file_path);

#endif // GITADD_H
