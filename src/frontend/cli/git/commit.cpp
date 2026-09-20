#include "git/commit.h"

#include "git2.h"

#include "retcodes.h"

int vault_git_commit(const std::filesystem::path& repo_path, const std::string& message) {
    int retcode = VAULT_SUCCESS;

    git_repository* repo {};
    git_index* index {};
    git_tree* tree {};
    git_signature* signature {};

    git_oid tree_oid {};
    git_oid new_commit_oid {};
    git_oid parent_commit_oid {};

    git_commit* parent_commit {};

    const git_commit* parents[1] {};
    size_t parent_count {};

    // Open the repository.
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
        goto epilogue;
    }

    // Retrieve the index of the repository.
    error = git_repository_index(&index, repo);
    if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
        goto epilogue;
    }

    // Obtain our signature.
    error = git_signature_default(&signature, repo);
    if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
        goto epilogue;
    }

    // Retrieve the tree for the index.
    error = git_index_write_tree(&tree_oid, index);
    if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
        goto epilogue;
    }

    error = git_tree_lookup(&tree, repo, &tree_oid);
    if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
        goto epilogue;
    }

    // Retrieve the previous commit, if any.
    error = git_reference_name_to_id(&parent_commit_oid, repo, "HEAD");
    if (error == GIT_ENOTFOUND) {
        // No HEAD yet, this is the first commit.
        parent_count = 0;
    } else if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
        goto epilogue;
    } else {
        // We have a parent commit.
        error = git_commit_lookup(&parent_commit, repo, &parent_commit_oid);
        if (error < 0) {
            retcode = VAULT_GIT_COMMIT_ERROR;
            goto epilogue;
        }

        parents[0] = parent_commit;
        parent_count = 1;
    }

    // Actually create the commit.
    error = git_commit_create(&new_commit_oid, repo, "HEAD", signature, signature, "UTF-8", message.c_str(), tree,
                              parent_count, parents);
    if (error < 0) {
        retcode = VAULT_GIT_COMMIT_ERROR;
    }

epilogue:
    git_commit_free(parent_commit);
    git_signature_free(signature);
    git_tree_free(tree);
    git_index_free(index);
    git_repository_free(repo);

    return retcode;
}
