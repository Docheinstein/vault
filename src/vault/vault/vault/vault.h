#ifndef VAULT_H
#define VAULT_H

#include <string>
#include <vector>

struct Vault {
    bool save(const std::string& path) const;
    bool load(const std::string& path);

    std::string to_json() const;
    bool parse_json(const std::string& json);

    struct Field {
        std::string name {};
        bool hidden {};
    };

    struct Entry {
        std::vector<std::string> values {};
    };

    std::vector<Field> fields {};
    std::vector<Entry> entries {};
};

#endif // VAULT_H
