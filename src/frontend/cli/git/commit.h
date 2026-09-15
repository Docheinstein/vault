#ifndef GITCOMMIT_H
#define GITCOMMIT_H

#include <filesystem>

int vault_git_commit(const std::filesystem::path& repo_path, const std::string& message);

#endif // GITCOMMIT_H
