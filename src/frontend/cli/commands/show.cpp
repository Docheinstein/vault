#include "commands/show.h"

#include <optional>
#include <string>

#include "args/args.h"

#include "vault/vault/secret.h"
#include "vault/vault/vault.h"

#include "utils/cli.h"
#include "utils/ui.h"
#include "utils/vault.h"

#include "result.h"

namespace {
std::string get_time_as_string(const time_t& time) {
    std::stringstream ss {};
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
    return ss.str();
}
} // namespace

VaultCommandResult command_show(int argc, char** argv) {
    struct {
        std::optional<std::string> id {};
        std::optional<std::string> vault_path {};
        bool when {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.id, "id").required(false).help("id of the entry");
    parser.add_argument(args.vault_path, "--vault-path", "-p").required(false).help("vault path (default is ~/.vault)");
    parser.add_argument(args.when, "--when", "-w").required(false).help("show the creation and modification time");

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

    const auto secrets = load_identified_vault_secrets(vault_path, *vault_key, true);

    if (args.id) {
        const auto& secret_path = get_secret_by_identifier(vault_path, *args.id);
        if (!secret_path) {
            return VAULT_SECRET_LOAD_ERROR;
        }

        const auto secret = load_secret(*secret_path, *vault_key);
        if (!secret) {
            return VAULT_SECRET_LOAD_ERROR;
        }

        secure_cout << *args.id << " " << get_secret_colorized_name(secret->name) << std::endl;
        secure_cout << secret->content << std::endl;

        if (args.when) {
            secure_cout << "Modified at: " << get_time_as_string(secret->modification_time) << std::endl;
        }
    } else {
        for (uint32_t i = 0; i < secrets.size(); i++) {
            const auto& [identifier, secret] = secrets[i];
            secure_cout << identifier << " " << get_secret_colorized_name(secret.name) << std::endl;
            secure_cout << secret.content << std::endl;

            if (args.when) {
                secure_cout << "Modified at: " << get_time_as_string(secret.modification_time) << std::endl;
            }

            if (i != secrets.size() - 1) {
                secure_cout << std::endl;
            }
        }
    }

    return VAULT_SUCCESS;
}
