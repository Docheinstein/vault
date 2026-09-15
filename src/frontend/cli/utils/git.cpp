#include "utils/git.h"

#include "git2.h"

bool has_git_repository(const std::filesystem::path& path) {
    git_repository* repo;
    int error = git_repository_open(&repo, path.c_str());
    git_repository_free(repo);
    return error >= 0;
}
