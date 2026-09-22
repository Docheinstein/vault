#include "commands/edit.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

#include "utils/cli.h"
#include "utils/ui.h"
#include "utils/vault.h"

#ifdef ENABLE_GIT
#include "utils/git.h"

#include "git/add.h"
#include "git/commit.h"
#endif

#include "result.h"

VaultCommandResult command_edit(int argc, char** argv) {
    struct {
        std::optional<std::string> id {};
        std::optional<std::string> vault_path {};
        bool multiline {};

    } args;

    Args::Parser parser {};
    parser.add_argument(args.id, "id").required(false).help("id of the entry");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");
    parser.add_argument(args.multiline, "--multiline", "-m").required(false).help("multiline");

    if (!parser.parse(argc, argv)) {
        return VAULT_COMMAND_ARGS_PARSE_ERROR;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vault_path();

    const std::filesystem::path vault_master_file_path = get_vault_master_file_path(vault_path);

    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        return VAULT_STDIN_ERROR;
    }

    const auto vault_key = load_vault(vault_master_file_path, *vault_password);
    if (!vault_key) {
        return VAULT_VAULT_LOAD_ERROR;
    }

    const std::string id = args.id ? *args.id : read_line_with_prompt("ID: ");
    if (id.empty()) {
        return VAULT_STDIN_ERROR;
    }

    const auto& secret_path = get_secret_by_identifier(vault_path, *args.id);
    if (!secret_path) {
        return VAULT_SECRET_LOAD_ERROR;
    }

    auto secret = load_secret(*secret_path, *vault_key);
    if (!secret) {
        return VAULT_SECRET_LOAD_ERROR;
    }

    secure_cout << std::endl << *args.id << " " << get_secret_colorized_name(secret->name) << std::endl;
    secure_cout << secret->content << std::endl << std::endl;

    if (args.multiline) {
        auto secret_content = read_multiline_with_prompt_secure("Enter content and press Ctrl+D when finished\n");
        if (!secret_content) {
            return VAULT_STDIN_ERROR;
        }
        secret->content = std::move(*secret_content);
    } else {
        auto secret_content = read_hidden_line_with_prompt_secure("Enter new password: ");
        if (!secret_content) {
            return VAULT_STDIN_ERROR;
        }

        const auto secret_content_again = read_hidden_line_with_prompt_secure("Retype new password: ");
        if (!secret_content_again) {
            return VAULT_STDIN_ERROR;
        }

        if (secret_content->size() != secret_content_again->size() ||
            sodium_memcmp(secret_content->data(), secret_content_again->data(), secret_content_again->size()) != 0) {
            return VAULT_PASSWORD_MISMATCH_ERROR;
        }

        secret->content = std::move(*secret_content);
    }

    const auto save_secret_result = save_secret(vault_path, *vault_key, *secret, true);

    if (!save_secret_result) {
        return VAULT_SECRET_SAVE_ERROR;
    }

#ifdef ENABLE_GIT
    if (has_git_repository(vault_path)) {
        int git_retcode = vault_git_add(vault_path, *save_secret_result);
        if (git_retcode != VAULT_SUCCESS) {
            return {git_retcode, get_git_error()};
        }

        git_retcode = vault_git_commit(vault_path, "Edit secret " + get_secret_short_name(*save_secret_result));
        if (git_retcode != VAULT_SUCCESS) {
            return {git_retcode, get_git_error()};
        }
    }
#endif

    return VAULT_SUCCESS;
}
