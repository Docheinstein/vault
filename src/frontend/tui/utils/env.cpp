#include "utils/env.h"

const char* get_default_home_path() {
    if (const char* const home_path = getenv("HOME")) {
        return home_path;
    }

    return "";
}

std::filesystem::path get_default_vaults_path() {
    const std::filesystem::path vaults_path = get_default_home_path();
    return vaults_path / ".vault";
}
