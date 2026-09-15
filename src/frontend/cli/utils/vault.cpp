#include "utils/vault.h"

#include "vault/vault/secret.h"

namespace {
const char* get_default_home_path() {
    if (const char* const home_path = getenv("HOME")) {
        return home_path;
    }

    return "";
}
} // namespace

std::filesystem::path get_default_vault_path() {
    const std::filesystem::path vaults_path = get_default_home_path();
    return vaults_path / ".vault";
}

std::filesystem::path get_vault_master_file_path(const std::filesystem::path& vault_path) {
    return vault_path / ".vault";
}

std::vector<std::string> get_vault_secrets(const std::filesystem::path& vault_path) {
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

std::string get_secret_short_name(const std::filesystem::path& secret_path) {
    return secret_path.filename().string().substr(0, 6);
}
