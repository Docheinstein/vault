#include "git/init.h"

#include "git2.h"

#include "retcodes.h"

int git_init(const std::filesystem::path& repo_path, const std::string& remote_url, const std::string& remote_name) {
    // Initialize the repo.
    git_repository* repo;

    int error = git_repository_init(&repo, repo_path.c_str(), false);
    if (error < 0) {
        return VAULT_GIT_INIT_ERROR;
    }

    // Set the remote.
    error = git_remote_set_url(repo, remote_name.c_str(), remote_url.c_str());
    if (error < 0) {
        return VAULT_GIT_SET_REMOTE_ERROR;
    }

    git_repository_free(repo);

    return VAULT_SUCCESS;
}
