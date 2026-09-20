#ifndef GITPULL_H
#define GITPULL_H

#include <filesystem>

int vault_git_pull(const std::filesystem::path& repo_path, const std::string& remote_name = "origin");

#endif // GITPULL_H
