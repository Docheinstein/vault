#include "vault/utils/vaults.h"

#include <cstring>

#include "vault/fs/fileio.h"

bool is_secret_file(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    if (path.filename() == ".vault") {
        return false;
    }

    constexpr unsigned long long VAULT_MAGIC_BYTES_SIZE = 4;
    const unsigned char VAULT_MAGIC_BYTES[VAULT_MAGIC_BYTES_SIZE] = "VLT";

    const auto magic_bytes_result = read_binary_file(path.string(), 4);
    if (!magic_bytes_result.has_value()) {
        return false;
    }

    return std::memcmp(magic_bytes_result->data(), VAULT_MAGIC_BYTES, VAULT_MAGIC_BYTES_SIZE) == 0;
}
