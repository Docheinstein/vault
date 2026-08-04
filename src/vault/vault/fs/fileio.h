#ifndef FILEIO_H
#define FILEIO_H

#include <cstddef>
#include <expected>
#include <string>
#include <vector>

enum class FileError {
    OpenError,
    StatError,
    IOError,
};

using read_file_result = std::expected<std::vector<unsigned char>, FileError>;
using write_file_result = std::expected<void, FileError>;

read_file_result read_file(const std::string& filename);
read_file_result read_file(const std::string& filename, size_t length);

write_file_result write_file(const std::string& filename, const void* data, size_t length);

#endif // FILEIO_H
