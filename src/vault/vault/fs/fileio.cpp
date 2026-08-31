#include "vault/fs/fileio.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace {
size_t file_size(const std::string& filename) {
    struct stat st {};
    if (stat(filename.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return 0;
    }

    return st.st_size;
}
} // namespace

read_text_file_result read_text_file(const std::string& filename) {
    std::ifstream ifs {filename, std::ios::in};
    if (!ifs) {
        return std::unexpected {FileError::OpenError};
    }

    const size_t size = file_size(filename);
    if (!size) {
        return std::unexpected {FileError::StatError};
    }

    std::stringstream out {};
    out << ifs.rdbuf();

    if (ifs.fail()) {
        return std::unexpected {FileError::IOError};
    }

    return out.str();
}

read_binary_file_result read_binary_file(const std::string& filename) {
    std::ifstream ifs {filename, std::ios::in | std::ios::binary};
    if (!ifs) {
        return std::unexpected {FileError::OpenError};
    }

    const size_t size = file_size(filename);
    if (!size) {
        return std::unexpected {FileError::StatError};
    }

    std::vector<unsigned char> out {};
    out.resize(size);

    ifs.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
    if (ifs.fail()) {
        return std::unexpected {FileError::IOError};
    }

    return out;
}

read_binary_file_result read_binary_file(const std::string& filename, const size_t length) {
    std::ifstream ifs {filename, std::ios::in | std::ios::binary};
    if (!ifs) {
        return std::unexpected {FileError::OpenError};
    }

    std::vector<unsigned char> out {};
    out.resize(length);

    ifs.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(length));
    if (ifs.fail()) {
        return std::unexpected {FileError::IOError};
    }

    return out;
}

write_binary_file_result write_binary_file(const std::string& filename, const void* data, const size_t length) {
    std::ofstream ofs {filename, std::ios::out | std::ios::binary};
    if (!ofs) {
        return std::unexpected {FileError::OpenError};
    }

    ofs.write(static_cast<const char*>(data), static_cast<std::streamsize>(length));
    if (ofs.fail()) {
        return std::unexpected {FileError::IOError};
    }

    return {};
}
