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

using read_text_file_result = std::expected<std::string, FileError>;

using read_binary_file_result = std::expected<std::vector<unsigned char>, FileError>;
using write_binary_file_result = std::expected<void, FileError>;

read_text_file_result read_text_file(const std::string& filename);

read_binary_file_result read_binary_file(const std::string& filename);
read_binary_file_result read_binary_file(const std::string& filename, size_t length);

write_binary_file_result write_binary_file(const std::string& filename, const void* data, size_t length);

#endif // FILEIO_H
