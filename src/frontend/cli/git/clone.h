#ifndef GITCLONE_H
#define GITCLONE_H

#include <filesystem>
#include <string>

int vault_git_clone(const std::filesystem::path& repo_path, const std::string& remote_url);

#endif // GITCLONE_H
