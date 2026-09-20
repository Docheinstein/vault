#include "git/remove.h"

#include "git2.h"

#include "retcodes.h"

int vault_git_remove(const std::filesystem::path& repo_path, const std::filesystem::path& file_path) {
    int retcode = VAULT_SUCCESS;

    git_repository* repo {};
    git_index* index {};

    std::string relative_file_path {};

    // Open the repository.
    int error = git_repository_open(&repo, repo_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_REMOVE_ERROR;
        goto epilogue;
    }

    // Retrieve the index of the repository.
    error = git_repository_index(&index, repo);
    if (error < 0) {
        retcode = VAULT_GIT_REMOVE_ERROR;
        goto epilogue;
    }

    relative_file_path = std::filesystem::relative(file_path, repo_path).string();

    // Remove the file from the index.
    error = git_index_remove_bypath(index, relative_file_path.c_str());
    if (error < 0) {
        retcode = VAULT_GIT_REMOVE_ERROR;
        goto epilogue;
    }

    // Write the index to disk.
    error = git_index_write(index);
    if (error < 0) {
        retcode = VAULT_GIT_REMOVE_ERROR;
    }

epilogue:
    git_index_free(index);
    git_repository_free(repo);

    return retcode;
}
