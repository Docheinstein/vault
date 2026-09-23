#include "utils/vault.h"

#include <algorithm>
#include <unordered_set>

#include "vault/vault/secret.h"

namespace {
const char* get_default_home_path() {
    if (const char* const home_path = getenv("HOME")) {
        return home_path;
    }

    return "";
}

bool compare_char_case_insensitive(const char lhs, const char rhs) {
    return std::tolower(static_cast<unsigned char>(lhs)) < std::tolower(static_cast<unsigned char>(rhs));
}

uint32_t get_shortest_unique_prefix(const std::vector<std::string>& strings, uint32_t initial_prefix_length) {
    bool all_unique = true;

    auto prefix_length = static_cast<int32_t>(initial_prefix_length);

    while (all_unique && prefix_length > 0) {
        --prefix_length;

        std::unordered_set<std::string> unique_identifiers {};

        for (const auto& string : strings) {
            std::string truncated_identifier = string.substr(0, prefix_length);
            if (!unique_identifiers.contains(truncated_identifier)) {
                unique_identifiers.insert(std::move(truncated_identifier));
            } else {
                all_unique = false;
                break;
            }
        }
    }

    return prefix_length + 1;
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

    return secrets_path;
}

std::optional<std::string> get_secret_by_identifier(const std::filesystem::path& vault_path,
                                                    const std::string& identifier) {
    std::optional<std::string> secret_path {};

    for (const auto& iter : std::filesystem::directory_iterator(vault_path)) {
        if (is_secret_file(iter.path()) && iter.path().filename().string().starts_with(identifier)) {
            if (secret_path) {
                // Ambiguity: multiple entries with the same prefix.
                return std::nullopt;
            }
            secret_path = iter.path().string();
        }
    }

    return secret_path;
}

std::string get_secret_short_name(const std::filesystem::path& secret_path, uint32_t name_length) {
    return secret_path.filename().string().substr(0, name_length);
}

std::vector<std::pair<std::string, Secret>>
load_identified_vault_secrets(const std::filesystem::path& vault_path, const VaultKey& vault_key, bool sort_by_name) {
    std::vector<std::string> vault_secrets = get_vault_secrets(vault_path);

    std::vector<std::pair<std::string, Secret>> identified_secrets {};
    std::vector<std::string> identifiers {};
    std::vector<Secret> secrets {};

    uint32_t identifier_length = 6;

    // Load all the secrets.
    for (const auto& secret_path_str : vault_secrets) {
        const std::filesystem::path secret_path(secret_path_str);
        if (auto secret = load_secret(secret_path, vault_key)) {
            identifiers.push_back(get_secret_short_name(secret_path, identifier_length));
            secrets.push_back(std::move(*secret));
        }
    }

    // Use the shortest possible identifier.
    identifier_length = get_shortest_unique_prefix(identifiers, identifier_length);

    for (uint32_t i = 0; i < secrets.size(); ++i) {
        identified_secrets.emplace_back(std::move(identifiers[i]).substr(0, identifier_length), std::move(secrets[i]));
    }

    // Eventually sort them.
    if (sort_by_name) {
        std::ranges::sort(identified_secrets, [](const std::pair<std::string, Secret>& lhs,
                                                 const std::pair<std::string, Secret>& rhs) {
            const Secret& lhs_secret = lhs.second;
            const Secret& rhs_secret = rhs.second;
            return std::lexicographical_compare(lhs_secret.name.data(), lhs_secret.name.data() + lhs_secret.name.size(),
                                                rhs_secret.name.data(), rhs_secret.name.data() + rhs_secret.name.size(),
                                                compare_char_case_insensitive);
        });
    }

    return identified_secrets;
}
