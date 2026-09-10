
#include <iostream>

#include "vault/utils/strings.h"
#include "vault/vault/secret.h"

std::vector<std::string> get_all_secrets(const std::filesystem::path& vault_path) {
    std::vector<std::string> secrets_path {};
    for (const auto& iter : std::filesystem::directory_iterator(vault_path)) {
        if (is_secret_file(iter.path())) {
            secrets_path.push_back(iter.path().string());
        }
    }

    std::sort(secrets_path.begin(), secrets_path.end(), [](const std::string& a, const std::string& b) {
        return a < b;
    });

    return secrets_path;
}
