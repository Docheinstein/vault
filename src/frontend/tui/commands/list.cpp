#include "commands/list.h"

#include <iostream>
#include <optional>
#include <string>

#include "args/args.h"

#include "vault/utils/vaults.h"

#include "utils/env.h"

#include "commands/exitcodes.h"

void command_list(int argc, char** argv) {
    struct {
        std::optional<std::string> vaults_path {};
    } args;

    Args::Parser parser {};
    parser.add_argument(args.vaults_path, "--path").required(false).help("vaults path (default is ~/.vault)");

    if (!parser.parse(argc, argv)) {
        exit(EXIT_UNKNOWN_COMMAND);
    }

    const std::filesystem::path vaults_path =
        args.vaults_path.has_value() ? std::filesystem::path {*args.vaults_path} : get_default_vaults_path();

    for (const auto& iter : std::filesystem::directory_iterator(vaults_path)) {
        if (is_vault_file(iter.path())) {
            std::cout << iter.path().filename().string() << std::endl;
        }
    }
}
