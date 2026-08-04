#include "commands/destroy.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "utils/env.h"
#include "utils/prompt.h"
#include "utils/vaults.h"

#include "commands/exitcodes.h"

void command_destroy(int argc, char** argv) {
    struct {
        std::optional<std::string> vault_name {};
        std::optional<std::string> vaults_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vault_name, "name").required(false).help("vault name");
    parser.add_argument(args.vaults_path, "--path").required(false).help("vaults path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::string vault_name = args.vault_name.has_value() ? *args.vault_name : read_vault_name();
    const std::filesystem::path vaults_path =
        args.vaults_path.has_value() ? std::filesystem::path {*args.vaults_path} : get_default_vaults_path();
    const std::filesystem::path vault_path = vaults_path / vault_name;

    Vault vault {};

    const bool ok = std::filesystem::exists(vault_path) && std::filesystem::is_regular_file(vault_path) &&
                    std::filesystem::remove(vault_path);

    if (!ok) {
        std::cerr << "ERROR: failed to destroy vault '" << vault_name << "'" << std::endl;
        exit(EXIT_IO_ERROR);
    }
}
