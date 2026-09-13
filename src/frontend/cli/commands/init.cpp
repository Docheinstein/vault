#include "commands/init.h"
#include "git/init.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/vault.h"

#include "utils/cli.h"
#include "utils/vaults.h"

#include "retcodes.h"

int command_init(int argc, char** argv) {
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

    if (std::filesystem::exists(vault_master_file_path)) {
        if (!read_yes_no_with_prompt("Vault already exists: overwrite? [y/N] ", false)) {
            return VAULT_SUCCESS;
        }
    }

    const auto vault_password = read_hidden_line_with_prompt_secure("Vault password: ");
    if (!vault_password) {
        return VAULT_STDIN_ERROR;
    }

    const auto save_vault_result = save_vault(vault_master_file_path, *vault_password);
    if (!save_vault_result) {
        return VAULT_VAULT_SAVE_ERROR;
    }

#ifdef ENABLE_GIT
    // Eventually init git repository.
    if (read_yes_no_with_prompt("Initialize git repository? [Y/n] ", true)) {
        const std::string git_repo_path = read_line_with_prompt("Git remote URL: ");
        git_init(vault_path, git_repo_path);
    }
#endif

    return VAULT_SUCCESS;
}
