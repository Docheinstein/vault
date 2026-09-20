#ifndef GITINIT_H
#define GITINIT_H

#include <filesystem>
#include <string>

int vault_git_init(const std::filesystem::path& repo_path, const std::string& remote_url,
                   const std::string& remote_name = "origin");

#endif // GITINIT_H
