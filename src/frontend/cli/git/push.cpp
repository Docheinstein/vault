#include "git/push.h"

#include "git2.h"

#include "utils/git.h"

#include "retcodes.h"

int vault_git_push(const std::filesystem::path& repo_path, const std::string& remote_name) {
    int retcode = VAULT_SUCCESS;

    git_repository* repo {};
    git_remote* remote {};

    git_push_options push_options {};

    git_reference* head {};
    git_reference* upstream_head {};

    const git_oid* head_oid {};

    const char* head_refname {};
    std::string head_shorthand {};

    std::string upstream_refname {};
    std::string upstream_shorthand {};

    git_buf configured_upstream_refname {};

    git_strarray refspecs {};
    std::string refspec {};
    char* refspec_cstr {};

    // Open the repository.
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
        goto epilogue;
    }

    // Retrieve the remote.
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

    head_refname = git_reference_name(head);
    head_shorthand = git_reference_shorthand(head);
    head_oid = git_reference_target(head);

    refspec = std::string {head_refname};
    refspec_cstr = refspec.data();
    refspecs.strings = &refspec_cstr;
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
    error = git_branch_upstream_name(&configured_upstream_refname, repo, head_refname);
    if (error == GIT_ENOTFOUND) {
        upstream_refname = "refs/remotes/" + remote_name + "/" + head_shorthand;

        error = git_reference_create(&upstream_head, repo, upstream_refname.c_str(), head_oid, 1, nullptr);
        if (error < 0) {
            retcode = VAULT_GIT_PUSH_ERROR;
            goto epilogue;
        }

        upstream_shorthand = remote_name + "/" + head_shorthand;
        error = git_branch_set_upstream(head, upstream_shorthand.c_str());
        if (error < 0) {
            retcode = VAULT_GIT_PUSH_ERROR;
        }
    } else if (error < 0) {
        retcode = VAULT_GIT_PUSH_ERROR;
    }

epilogue:
    git_buf_dispose(&configured_upstream_refname);
    git_reference_free(upstream_head);
    git_reference_free(head);
    git_remote_free(remote);
    git_repository_free(repo);

    return retcode;
}