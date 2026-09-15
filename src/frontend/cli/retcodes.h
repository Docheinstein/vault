#ifndef RETCODES_H
#define RETCODES_H

constexpr int VAULT_SUCCESS = 0;
constexpr int VAULT_BOOTSTRAP_FAILED = 1;
constexpr int VAULT_UNRECOGNIZED_COMMAND = 2;
constexpr int VAULT_COMMAND_ARGS_PARSE_ERROR = 3;
constexpr int VAULT_STDIN_ERROR = 4;
constexpr int VAULT_PASSWORD_MISMATCH_ERROR = 5;
constexpr int VAULT_VAULT_LOAD_ERROR = 6;
constexpr int VAULT_VAULT_SAVE_ERROR = 7;
constexpr int VAULT_SECRET_LOAD_ERROR = 8;
constexpr int VAULT_SECRET_SAVE_ERROR = 9;
constexpr int VAULT_SECRET_REMOVE_ERROR = 10;
constexpr int VAULT_INVALID_ID_ERROR = 11;
constexpr int VAULT_INVALID_SEARCH_PATTERN_ERROR = 12;

constexpr int VAULT_GIT_INIT_ERROR = 31;
constexpr int VAULT_GIT_SET_REMOTE_ERROR = 32;
constexpr int VAULT_GIT_OPEN_ERROR = 33;
constexpr int VAULT_GIT_ADD_ERROR = 34;
constexpr int VAULT_GIT_COMMIT_ERROR = 35;

#endif // RETCODES_H
