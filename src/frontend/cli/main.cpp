#include <filesystem>
#include <iostream>
#include <ranges>

#include "sodium.h"

#include "args/args.h"

#include "commands/add.h"
#include "commands/edit.h"
#include "commands/exitcodes.h"
#include "commands/init.h"
#include "commands/list.h"
#include "commands/remove.h"
#include "commands/search.h"
#include "commands/show.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: {add,create,destroy,list,remove,search,show,update}" << std::endl;
        return EXIT_SUCCESS;
    }

    sodium_init();

    const std::string command = argv[1];

    const int cmd_argc = argc - 2;
    char** const cmd_argv = &argv[2];

    if (command == "add") {
        command_add(cmd_argc, cmd_argv);
    } else if (command == "init") {
        command_init(cmd_argc, cmd_argv);
    } else if (command == "list") {
        command_list(cmd_argc, cmd_argv);
    } else if (command == "remove") {
        command_remove(cmd_argc, cmd_argv);
    } else if (command == "search") {
        command_search(cmd_argc, cmd_argv);
    } else if (command == "show") {
        command_show(cmd_argc, cmd_argv);
    } else if (command == "edit") {
        command_edit(cmd_argc, cmd_argv);
    } else {
        std::cout << "ERROR: unknown command '" << command << "'" << std::endl;
        return EXIT_UNKNOWN_COMMAND;
    }

    return EXIT_SUCCESS;
}