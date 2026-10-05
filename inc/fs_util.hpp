#ifndef __FS_UTIL_HPP__
#define __FS_UTIL_HPP__

#include <cstdint>
#include <string>
#include <vector>

std::vector<uint8_t> read_file(const std::string &path);
void write_file(const std::string &path, const std::string &data);
std::string sanitize_filename(const std::string &name);
std::string join_path(const std::string &dir, const std::string &name);

#endif // __FS_UTIL_HPP__
