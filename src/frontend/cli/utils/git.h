#ifndef GITUTILS_H
#define GITUTILS_H

#include <filesystem>

#include "git2.h"

bool has_git_repository(const std::filesystem::path& path);

int git_credentials_callback(git_credential** out, const char* url, const char* username_from_url,
                             unsigned int allowed_types, void* payload);

#endif // GITUTILS_H
