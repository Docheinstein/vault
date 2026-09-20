#include "git/clone.h"

#include <iostream>

#include "git2.h"

#include "utils/git.h"

#include "retcodes.h"

int vault_git_clone(const std::filesystem::path& repo_path, const std::string& remote_url) {
    int retcode = VAULT_SUCCESS;

    git_repository* repo {};

    git_clone_options clone_options {};

    int error = git_clone_options_init(&clone_options, GIT_CLONE_OPTIONS_VERSION);
    if (error < 0) {
        retcode = VAULT_GIT_CLONE_ERROR;
        goto epilogue;
    }

    // Setup the credentials callbacks.
    clone_options.fetch_opts.callbacks.credentials = git_credentials_callback;

    // Clone the remote.
    error = git_clone(&repo, remote_url.c_str(), repo_path.c_str(), &clone_options);
    if (error < 0) {
        retcode = VAULT_GIT_CLONE_ERROR;
        goto epilogue;
    }

    std::cout << "Cloned successfully into " << repo_path.string() << std::endl;

epilogue:
    git_repository_free(repo);

    return retcode;
}
