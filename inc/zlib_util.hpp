#ifndef __ZLIB_UTIL_HPP__
#define __ZLIB_UTIL_HPP__

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

std::string zlib_inflate(const std::string &compressed);
std::string zlib_inflate(const std::vector<uint8_t> &compressed);

class ZlibError : public std::runtime_error {
	public:
		explicit ZlibError(const std::string &message);
};

#endif // __ZLIB_UTIL_HPP__
