#include "commands/pull.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "utils/git.h"
#include "utils/vault.h"

#include "git/pull.h"

#include "result.h"

VaultCommandResult command_pull(int argc, char** argv) {
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

    if (!has_git_repository(vault_path)) {
        return VAULT_GIT_PULL_ERROR;
    }

    const int git_retcode = vault_git_pull(vault_path);
    if (git_retcode != VAULT_SUCCESS) {
        return {git_retcode, get_git_error()};
    }

    return VAULT_SUCCESS;
}
