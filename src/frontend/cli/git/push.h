#ifndef GITPUSH_H
#define GITPUSH_H

#include <filesystem>
#include <string>

int vault_git_push(const std::filesystem::path& repo_path, const std::string& remote_name = "origin");

#endif // GITPUSH_H
