#include "vault/vault/vault.h"

#include "simdjson.h"

#include "vault/crypto/crypto.h"
#include "vault/fs/fileio.h"

#ifdef DEBUG_STORE_PLAINTEXT
bool Vault::save(const std::string& path, const std::string& password) const {
    const std::string vault_json = to_json();
    const char* const vault_json_str = vault_json.c_str();

    const auto result = write_file(path, vault_json_str, vault_json.size());
    return result.has_value();
}
#else
bool Vault::save(const std::string& path, const std::string& password) const {
    const std::string vault_json = to_json();
    const char* const vault_json_str = vault_json.c_str();

    const char* const password_str = password.c_str();

    const encrypt_result encrypt_result = encrypt(vault_json_str, password_str);
    if (!encrypt_result) {
        return false;
    }

    const auto result = write_file(path, encrypt_result->data(), encrypt_result->size());
    return result.has_value();
}
#endif

#ifdef DEBUG_STORE_PLAINTEXT
bool Vault::load(const std::string& path, const std::string& password) {
    const auto result = read_file(path);
    if (!result.has_value()) {
        return false;
    }

    const auto& content = result.value();
    const std::string content_str {content.begin(), content.end()};

    return parse_json(content_str);
}
#else

bool Vault::load(const std::string& path, const std::string& password) {
    const read_file_result read_result = read_file(path);
    if (!read_result) {
        return false;
    }

    const char* const password_str = password.c_str();

    const decrypt_result decrypt_result = decrypt(*read_result, password_str);
    if (!decrypt_result) {
        return false;
    }

    const std::string result(decrypt_result->begin(), decrypt_result->end());

    return parse_json(result);
}
#endif

std::string Vault::to_json() const {
    const auto num_fields = fields.size();
    const auto num_entries = entries.size();

    simdjson::builder::string_builder sb;
    sb.start_object();

    // Fields.
    sb.escape_and_append_with_quotes("fields");
    sb.append_colon();
    sb.start_array();
    for (uint32_t i = 0; i < num_fields; ++i) {
        const auto& field = fields[i];
        sb.start_object();
        sb.append_key_value("name", field.name);
        sb.append_comma();
        sb.append_key_value("hidden", field.hidden);
        sb.end_object();
        if (i < num_fields - 1) {
            sb.append_comma();
        }
    }
    sb.end_array();

    sb.append_comma();

    // Entries.
    sb.escape_and_append_with_quotes("entries");
    sb.append_colon();
    sb.start_array();
    for (uint32_t entry_idx = 0; entry_idx < num_entries; ++entry_idx) {
        const auto& entry = entries[entry_idx];
        const auto num_values = entry.values.size();
        sb.start_array();
        for (uint32_t value_idx = 0; value_idx < num_values; ++value_idx) {
            const auto& value = entry.values[value_idx];
            sb.append(value);
            if (value_idx < num_values - 1) {
                sb.append_comma();
            }
        }
        sb.end_array();
        if (entry_idx < num_entries - 1) {
            sb.append_comma();
        }
    }

    sb.end_array();
    sb.end_object();

    return std::string {sb};
}

bool Vault::parse_json(const std::string& json) {
#define SIMDJSON_ENSURE_SUCCESS(x)                                                                                     \
    if (x.error() != simdjson::SUCCESS) {                                                                              \
        std::cerr << "JSON error: failed to parse: " << #x << std::endl;                                               \
        return false;                                                                                                  \
    }

    simdjson::ondemand::parser parser;
    simdjson::padded_string padded_json = json;
    simdjson::ondemand::document doc = parser.iterate(padded_json);
    simdjson::ondemand::object root = doc.get_object();

    auto fields_element = root.find_field("fields");
    SIMDJSON_ENSURE_SUCCESS(fields_element);

    auto fields_array = fields_element.get_array();
    SIMDJSON_ENSURE_SUCCESS(fields_array);

    for (auto field_element : fields_array) {
        auto field_object = field_element.get_object();
        SIMDJSON_ENSURE_SUCCESS(field_object);

        Field field {};

        auto field_name_element = field_object.find_field("name");
        SIMDJSON_ENSURE_SUCCESS(field_name_element);

        auto field_name_string = field_name_element.get_string();
        SIMDJSON_ENSURE_SUCCESS(field_name_string);

        field.name = field_name_string.value();

        auto field_hidden_element = field_object.find_field("hidden");
        SIMDJSON_ENSURE_SUCCESS(field_hidden_element);

        auto field_hidden_bool = field_hidden_element.get_bool();
        SIMDJSON_ENSURE_SUCCESS(field_hidden_bool);

        field.hidden = field_hidden_bool.value();

        fields.push_back(std::move(field));
    }

    auto entries_element = root.find_field("entries");
    SIMDJSON_ENSURE_SUCCESS(entries_element);

    auto entries_array = entries_element.get_array();
    SIMDJSON_ENSURE_SUCCESS(entries_array);

    for (auto entry_element : entries_array) {
        auto entry_values = entry_element.get_array();
        SIMDJSON_ENSURE_SUCCESS(entry_values);

        Entry entry {};

        for (auto entry_value_element : entry_values) {
            auto entry_value_string = entry_value_element.get_string();
            SIMDJSON_ENSURE_SUCCESS(entry_value_string);

            entry.values.emplace_back(entry_value_string.value());
        }

        entries.push_back(std::move(entry));
    }

#undef SIMDJSON_ENSURE_SUCCESS
    return true;
}