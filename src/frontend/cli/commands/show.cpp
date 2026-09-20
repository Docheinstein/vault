#include "commands/show.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

#include "utils/cli.h"
#include "utils/colors.h"
#include "utils/vault.h"

#include "result.h"

VaultCommandResult command_show(int argc, char** argv) {
    struct {
        std::optional<uint32_t> id {};
        std::optional<std::string> vault_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");
    parser.add_argument(args.id, "id").required(false).help("id of the entry");

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

    const std::vector<std::string> secrets_paths = get_vault_secrets(vault_path);

    if (args.id) {
        if (*args.id >= secrets_paths.size()) {
            return VAULT_INVALID_ID_ERROR;
        }

        const auto& secret_path = secrets_paths[*args.id];

        const auto secret = load_secret(secret_path, *vault_key);
        if (!secret) {
            return VAULT_SECRET_LOAD_ERROR;
        }

        secure_cout << *args.id << ". " << CYAN << secret->name << RESET << std::endl;
        secure_cout << secret->content << std::endl;
    } else {
        for (uint32_t i = 0; i < secrets_paths.size(); i++) {
            const auto& secret_path = secrets_paths[i];

            auto secret = load_secret(secret_path, *vault_key);
            if (!secret) {
                return VAULT_SECRET_LOAD_ERROR;
            }

            secure_cout << i << ". " << CYAN << secret->name << RESET << std::endl;
            secure_cout << secret->content << std::endl;

            if (i != secrets_paths.size() - 1) {
                secure_cout << std::endl;
            }
        }
    }

    return VAULT_SUCCESS;
}
