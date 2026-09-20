#include "utils/git.h"

#include "git2.h"

#include "utils/cli.h"

bool has_git_repository(const std::filesystem::path& path) {
    git_repository* repo;
    int error = git_repository_open(&repo, path.c_str());
    git_repository_free(repo);
    return error >= 0;
}

int git_credentials_callback(git_credential** out, const char* url, const char* username_from_url,
                             unsigned int allowed_types, void* payload) {

    if (!(allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT)) {
        return GIT_PASSTHROUGH;
    }

    const std::string username =
        username_from_url != nullptr ? std::string(username_from_url) : read_line_with_prompt("Username: ");

    auto password = read_hidden_line_with_prompt_secure("Password: ");
    if (!password) {
        return GIT_EUSER;
    }

    constexpr unsigned char c_str_end = '\0';
    password->append(&c_str_end, 1);

    return git_credential_userpass_plaintext_new(out, username.c_str(),
                                                 reinterpret_cast<const char*>(password->data()));
}

std::string get_commit_short_name(const std::string& commit_name) {
    return commit_name.size() >= 6 ? commit_name.substr(0, 6) : commit_name;
}

std::string get_git_error() {
    return git_error_last() ? git_error_last()->message : nullptr;
}
