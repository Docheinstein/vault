#include "commands/remove.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/cli.h"
#include "utils/vault.h"

#ifdef ENABLE_GIT
#include "utils/git.h"

#include "git/commit.h"
#include "git/remove.h"
#endif

#include "result.h"

VaultCommandResult command_remove(int argc, char** argv) {
    struct {
        std::optional<std::string> id {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.id, "id").required(false).help("id of the entry");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

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

    if (!std::filesystem::remove(*secret_path)) {
        return VAULT_SECRET_REMOVE_ERROR;
    }

#ifdef ENABLE_GIT
    if (has_git_repository(vault_path)) {
        int git_retcode = vault_git_remove(vault_path, *secret_path);
        if (git_retcode != VAULT_SUCCESS) {
            return {git_retcode, get_git_error()};
        }

        git_retcode = vault_git_commit(vault_path, "Remove secret " + get_secret_short_name(*secret_path));
        if (git_retcode != VAULT_SUCCESS) {
            return {git_retcode, get_git_error()};
        }
    }
#endif

    return VAULT_SUCCESS;
}
