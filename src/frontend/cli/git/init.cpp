#include "git/init.h"

#include "git2.h"

#include "retcodes.h"

int vault_git_init(const std::filesystem::path& repo_path, const std::string& remote_url,
                   const std::string& remote_name) {
    int retcode = VAULT_SUCCESS;

    // Initialize the repository.
    git_repository* repo {};

    int error = git_repository_init(&repo, repo_path.c_str(), false);
    if (error < 0) {
        retcode = VAULT_GIT_INIT_ERROR;
        goto epilogue;
    }

    // Set the remote URL.
    if (!remote_url.empty()) {
        error = git_remote_set_url(repo, remote_name.c_str(), remote_url.c_str());
        if (error < 0) {
            retcode = VAULT_GIT_SET_REMOTE_ERROR;
        }
    }

epilogue:
    git_repository_free(repo);

    return retcode;
}
