#include "commands/clone.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "utils/cli.h"
#include "utils/git.h"
#include "utils/vault.h"

#include "git/clone.h"

#include "result.h"

VaultCommandResult command_clone(int argc, char** argv) {
    struct {
        std::optional<std::string> remote_url {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.remote_url, "url").required(false).help("URL of the remote vault repository");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        return VAULT_COMMAND_ARGS_PARSE_ERROR;
    }

    const std::filesystem::path vault_path =
        args.vault_path.has_value() ? std::filesystem::path {*args.vault_path} : get_default_vault_path();

    const std::string git_remote_url = args.remote_url ? *args.remote_url : read_line_with_prompt("URL: ");
    if (git_remote_url.empty()) {
        return VAULT_GIT_CLONE_ERROR;
    }

    const int git_retcode = vault_git_clone(vault_path, git_remote_url);
    if (git_retcode != VAULT_SUCCESS) {
        return {git_retcode, get_git_error()};
    }

    return VAULT_SUCCESS;
}
