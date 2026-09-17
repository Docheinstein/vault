#include <iostream>

#include "vault/init.h"

#include "commands/add.h"
#include "commands/edit.h"
#include "commands/init.h"
#include "commands/list.h"
#include "commands/remove.h"
#include "commands/search.h"
#include "commands/show.h"

#ifdef ENABLE_GIT
#include "commands/push.h"
#endif

#include "retcodes.h"

namespace {
std::string get_error_message(int retcode) {
    switch (retcode) {
    case VAULT_BOOTSTRAP_FAILED:
        return "ERROR: failed to initialize vault libraries";
    case VAULT_UNRECOGNIZED_COMMAND:
        return "ERROR: unrecognized command";
    case VAULT_COMMAND_ARGS_PARSE_ERROR:
        return "ERROR: unrecognized command arguments";
    case VAULT_STDIN_ERROR:
        return "ERROR: broken IO";
    case VAULT_PASSWORD_MISMATCH_ERROR:
        return "ERROR: passwords do not match";
    case VAULT_VAULT_LOAD_ERROR:
        return "ERROR: failed to load vault";
    case VAULT_VAULT_SAVE_ERROR:
        return "ERROR: failed to save vault";
    case VAULT_SECRET_LOAD_ERROR:
        return "ERROR: failed to load secret";
    case VAULT_SECRET_SAVE_ERROR:
        return "ERROR: failed to save secret";
    case VAULT_SECRET_REMOVE_ERROR:
        return "ERROR: failed to remove secret";
    case VAULT_INVALID_ID_ERROR:
        return "ERROR: invalid id";
    case VAULT_INVALID_SEARCH_PATTERN_ERROR:
        return "ERROR: invalid search pattern";
    case VAULT_GIT_INIT_ERROR:
        return "ERROR: failed to init git repository";
    case VAULT_GIT_SET_REMOTE_ERROR:
        return "ERROR: failed to set git repository remote url";
    case VAULT_GIT_OPEN_ERROR:
        return "ERROR: failed to open git repository";
    case VAULT_GIT_ADD_ERROR:
        return "ERROR: failed to add file to git repository";
    case VAULT_GIT_COMMIT_ERROR:
        return "ERROR: failed to commit file to git repository";
    case VAULT_GIT_PUSH_ERROR:
        return "ERROR: failed to push to remote";
    case VAULT_GIT_SET_UPSTREAM_ERROR:
        return "ERROR: failed to set upstream branch";
    default:
        return "ERROR: unknown error";
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
#ifdef ENABLE_GIT
        std::cout << "usage: {add,edit,init,list,push,remove,search,show}" << std::endl;
#else
        std::cout << "usage: {add,edit,init,list,remove,search,show}" << std::endl;
#endif
        return VAULT_SUCCESS;
    }

    if (!vault_init()) {
        std::cerr << get_error_message(VAULT_BOOTSTRAP_FAILED) << std::endl;
        return VAULT_BOOTSTRAP_FAILED;
    }

    const std::string_view command = argv[1];
    const int cmd_argc = argc - 2;
    char** const cmd_argv = &argv[2];

    int retcode = VAULT_UNRECOGNIZED_COMMAND;

    if (command == "add") {
        retcode = command_add(cmd_argc, cmd_argv);
    } else if (command == "edit") {
        retcode = command_edit(cmd_argc, cmd_argv);
    } else if (command == "init") {
        retcode = command_init(cmd_argc, cmd_argv);
    } else if (command == "list") {
        retcode = command_list(cmd_argc, cmd_argv);
    }
#ifdef ENABLE_GIT
    else if (command == "push") {
        retcode = command_push(cmd_argc, cmd_argv);
    }
#endif
    else if (command == "remove") {
        retcode = command_remove(cmd_argc, cmd_argv);
    } else if (command == "search") {
        retcode = command_search(cmd_argc, cmd_argv);
    } else if (command == "show") {
        retcode = command_show(cmd_argc, cmd_argv);
    }

    if (retcode != VAULT_SUCCESS) {
        std::cerr << get_error_message(retcode) << std::endl;
    }

    vault_deinit();

    return retcode;
}
