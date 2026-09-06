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

using ReadTextFileResult = std::expected<std::string, FileError>;

using ReadBinaryFileResult = std::expected<std::vector<unsigned char>, FileError>;
using WriteBinaryFileResult = std::expected<void, FileError>;

ReadTextFileResult read_text_file(const std::string& filename);

ReadBinaryFileResult read_binary_file(const std::string& filename);
ReadBinaryFileResult read_binary_file(const std::string& filename, size_t length);

WriteBinaryFileResult write_binary_file(const std::string& filename, const void* data, size_t length);

#endif // FILEIO_H
