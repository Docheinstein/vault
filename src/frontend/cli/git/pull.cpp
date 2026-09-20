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

    git_fetch_options fetch_options {};
    git_checkout_options checkout_options {};

    git_reference* head {};
    git_reference* upstream_head {};
    git_reference* new_head {};

    git_annotated_commit* upstream_commit {};
    const git_annotated_commit* merge_heads[1] {};

    git_merge_analysis_t merge_analysis {};
    git_merge_preference_t merge_preference {};

    git_object* upstream_object {};

    const git_oid* head_oid {};
    const git_oid* upstream_oid {};

    size_t commits_ahead {};
    size_t commits_behind {};

    std::string head_shorthand {};

    // Open the repository.
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Retrieve the remote.
    error = git_remote_lookup(&remote, repo, remote_name.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_fetch_options_init(&fetch_options, GIT_FETCH_OPTIONS_VERSION);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Setup the credentials callbacks.
    fetch_options.callbacks.credentials = git_credentials_callback;

    // Fetch the remote branches.
    error = git_remote_fetch(remote, nullptr, &fetch_options, "fetch");
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

    error = git_annotated_commit_from_ref(&upstream_commit, repo, upstream_head);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    merge_heads[0] = upstream_commit;

    // Check how we can merge the local branch with the remote branch.
    error = git_merge_analysis(&merge_analysis, &merge_preference, repo, merge_heads, 1);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    if (merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        std::cout << "Already up to date" << std::endl;
        goto epilogue;
    }

    if (!(merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    // Fast-forward: move the local branch to the fetched commit and update the working tree.
    upstream_oid = git_annotated_commit_id(upstream_commit);
    head_oid = git_reference_target(head);

    // Count the number of commits separating the local from the remote branch.
    error = git_graph_ahead_behind(&commits_ahead, &commits_behind, repo, head_oid, upstream_oid);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_object_lookup(&upstream_object, repo, upstream_oid, GIT_OBJECT_COMMIT);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_checkout_options_init(&checkout_options, GIT_CHECKOUT_OPTIONS_VERSION);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;

    error = git_checkout_tree(repo, upstream_object, &checkout_options);
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    error = git_reference_set_target(&new_head, head, upstream_oid, "pull: fast-forward");
    if (error < 0) {
        retcode = VAULT_GIT_PULL_ERROR;
        goto epilogue;
    }

    std::cout << "Fast-forwarded " << head_shorthand << " to " << get_commit_short_name(git_oid_tostr_s(upstream_oid))
              << " (" << commits_behind << (commits_behind == 1 ? " new commit)" : " new commits)") << std::endl;

epilogue:
    git_object_free(upstream_object);
    git_annotated_commit_free(upstream_commit);
    git_reference_free(new_head);
    git_reference_free(upstream_head);
    git_reference_free(head);
    git_remote_free(remote);
    git_repository_free(repo);

    return retcode;
}
