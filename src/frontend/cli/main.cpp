#include <cstring>
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

#include <cstdlib>
#include <sys/wait.h>

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
        return command_add(cmd_argc, cmd_argv);
    }

    if (command == "init") {
        return command_init(cmd_argc, cmd_argv);
    }

    if (command == "list") {
        return command_list(cmd_argc, cmd_argv);
    }

    if (command == "remove") {
        return command_remove(cmd_argc, cmd_argv);
    }

    if (command == "search") {
        return command_search(cmd_argc, cmd_argv);
    }

    if (command == "show") {
        return command_show(cmd_argc, cmd_argv);
    }

    if (command == "edit") {
        return command_edit(cmd_argc, cmd_argv);
    }

    std::cout << "ERROR: unknown command '" << command << "'" << std::endl;
    return EXIT_UNKNOWN_COMMAND;
}
