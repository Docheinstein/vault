#ifndef GITUTILS_H
#define GITUTILS_H

#include <filesystem>

#include "git2.h"

bool has_git_repository(const std::filesystem::path& path);

int git_credentials_callback(git_credential** out, const char* url, const char* username_from_url,
                             unsigned int allowed_types, void* payload);

std::string get_commit_short_name(const std::string& commit_name);

std::string get_git_error();

#endif // GITUTILS_H
