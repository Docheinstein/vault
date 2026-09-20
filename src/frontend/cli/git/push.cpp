#include "git/push.h"

#include "git2.h"

#include "utils/git.h"

#include "retcodes.h"

int vault_git_push(const std::filesystem::path& repo_path, const std::string& remote_name) {
    int retcode = VAULT_SUCCESS;

    git_repository* repo {};
    git_remote* remote {};
    git_reference* head {};
    git_reference* tracking_ref {};

    const char* head_name {};

    git_strarray refspecs {};
    std::string refspec_str {};
    char* refspec {};

    git_push_options push_options {};

    std::string head_shorthand {};
    std::string head_tracking_refname {};
    std::string upstream_spec {};

    git_buf upstream_name {};

    // Open the repository.
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
        goto epilogue;
    }

    // Fetch the remote.
    error = git_remote_lookup(&remote, repo, remote_name.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
        goto epilogue;
    }

    // Resolve the current branch.
    error = git_repository_head(&head, repo);
    if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
        goto epilogue;
    }

    head_name = git_reference_name(head);

    refspec_str = std::string {head_name};
    refspec = refspec_str.data();
    refspecs.strings = &refspec;
    refspecs.count = 1;

    error = git_push_options_init(&push_options, GIT_PUSH_OPTIONS_VERSION);
    if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
        goto epilogue;
    }

    // Setup the credentials callbacks.
    push_options.callbacks.credentials = git_credentials_callback;

    error = git_remote_push(remote, &refspecs, &push_options);
    if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
        goto epilogue;
    }

    // Set the branch upstream (mirrors `git push --set-upstream`), if not done yet.
    error = git_branch_upstream_name(&upstream_name, repo, head_name);
    if (error == GIT_ENOTFOUND) {
        head_shorthand = git_reference_shorthand(head);
        head_tracking_refname = "refs/remotes/" + remote_name + "/" + head_shorthand;

        error = git_reference_create(&tracking_ref, repo, head_tracking_refname.c_str(), git_reference_target(head), 1,
                                     nullptr);
        if (error < 0) {
            retcode = VAULT_GIT_PUSH_ERROR;
            goto epilogue;
        }

        upstream_spec = remote_name + "/" + head_shorthand;
        error = git_branch_set_upstream(head, upstream_spec.c_str());
        if (error < 0) {
            retcode = VAULT_GIT_PUSH_ERROR;
        }
    } else if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
    }

epilogue:
    git_buf_dispose(&upstream_name);
    git_reference_free(tracking_ref);
    git_reference_free(head);
    git_remote_free(remote);
    git_repository_free(repo);

    return retcode;
}