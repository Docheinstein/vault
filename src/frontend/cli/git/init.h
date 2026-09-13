#ifndef GITINIT_H
#define GITINIT_H

#include <filesystem>

int git_init(const std::filesystem::path& repo_path, const std::string& remote_url,
             const std::string& remote_name = "origin");

#endif // GITINIT_H
