#ifndef GITREMOVE_H
#define GITREMOVE_H

#include <filesystem>

int vault_git_remove(const std::filesystem::path& repo_path, const std::filesystem::path& file_path);

#endif // GITREMOVE_H
