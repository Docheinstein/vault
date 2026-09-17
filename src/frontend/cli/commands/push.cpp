#include "commands/push.h"
#include "git/push.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/secret.h"

#include "utils/git.h"
#include "utils/vault.h"

#include "retcodes.h"

int command_push(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        return VAULT_COMMAND_ARGS_PARSE_ERROR;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vault_path();

    const std::filesystem::path vault_master_file_path = get_vault_master_file_path(vault_path);

    if (!has_git_repository(vault_path)) {
        return VAULT_GIT_PUSH_ERROR;
    }

    int git_retcode = vault_git_push(vault_path);
    if (git_retcode != VAULT_SUCCESS) {
        return git_retcode;
    }

    return VAULT_SUCCESS;
}
