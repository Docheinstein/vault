#include "git/pull.h"

#include <iostream>
#include <string>

#include "git2.h"

#include "utils/git.h"

#include "retcodes.h"

int vault_git_pull(const std::filesystem::path& repo_path, const std::string& remote_name) {
    int retcode = VAULT_SUCCESS;

    git_repository* repo {};
    git_remote* remote {};

    git_fetch_options fetch_opts {};
    git_checkout_options checkout_opts {};

    git_reference* head {};
    git_reference* upstream_head {};
    git_reference* new_head {};

    git_annotated_commit* tracking_commit {};
    const git_annotated_commit* merge_heads[1] {};

    git_merge_analysis_t analysis {};
    git_merge_preference_t preference {};

    git_object* target {};
    const git_oid* target_oid {};
    const git_oid* local_oid {};

    size_t ahead {};
    size_t behind {};

    std::string head_shorthand {};

    // Open the repository.
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Fetch the remote.
    error = git_remote_lookup(&remote, repo, remote_name.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Setup the credentials callbacks.
    fetch_opts.callbacks.credentials = git_credentials_callback;

    error = git_remote_fetch(remote, nullptr, &fetch_opts, "fetch");
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Resolve the current branch.
    error = git_repository_head(&head, repo);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    head_shorthand = git_reference_shorthand(head);

    // Resolve the remote upstream branch.
    error = git_branch_upstream(&upstream_head, head);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_annotated_commit_from_ref(&tracking_commit, repo, upstream_head);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    merge_heads[0] = tracking_commit;

    // Check how we can merge the local branch with the remote branch.
    error = git_merge_analysis(&analysis, &preference, repo, merge_heads, 1);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        std::cout << "Already up to date" << std::endl;
        goto epilogue;
    }

    if (!(analysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Fast-forward: move the local branch to the fetched commit and update the working tree.
    target_oid = git_annotated_commit_id(tracking_commit);
    local_oid = git_reference_target(head);

    // Count the number of commits separating the local from the remote branch.
    error = git_graph_ahead_behind(&ahead, &behind, repo, local_oid, target_oid);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_object_lookup(&target, repo, target_oid, GIT_OBJECT_COMMIT);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    error = git_checkout_tree(repo, target, &checkout_opts);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_reference_set_target(&new_head, head, target_oid, "pull: fast-forward");
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    std::cout << "Fast-forwarded " << head_shorthand << " to " << get_commit_short_name(git_oid_tostr_s(target_oid))
              << " (" << behind << (behind == 1 ? " new commit)" : " new commits)") << std::endl;

epilogue:
    git_object_free(target);
    git_annotated_commit_free(tracking_commit);
    git_reference_free(new_head);
    git_reference_free(upstream_head);
    git_reference_free(head);
    git_remote_free(remote);
    git_repository_free(repo);

    return retcode;
}
