#include "zlib_util.hpp"

#include <zlib.h>

#include <sstream>

ZlibError::ZlibError(const std::string &message)
	: std::runtime_error(message)
{
}

std::string zlib_inflate(const std::string &compressed)
{
	const std::vector<uint8_t> bytes(compressed.begin(), compressed.end());

	return zlib_inflate(bytes);
}

std::string zlib_inflate(const std::vector<uint8_t> &compressed)
{
	z_stream stream;
	int ret;
	std::string output;
	std::vector<uint8_t> buffer(16 * 1024);

	if (compressed.empty())
	{
		return std::string();
	}

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = 0;
	stream.next_in = Z_NULL;

	ret = inflateInit(&stream);
	if (ret != Z_OK)
	{
		throw ZlibError("zlib inflateInit failed");
	}

	stream.avail_in = static_cast<uInt>(compressed.size());
	stream.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(compressed.data()));

	do
	{
		size_t have;

		stream.avail_out = static_cast<uInt>(buffer.size());
		stream.next_out = buffer.data();
		ret = ::inflate(&stream, Z_NO_FLUSH);
		if (ret != Z_OK && ret != Z_STREAM_END)
		{
			std::ostringstream oss;

			inflateEnd(&stream);
			oss << "zlib inflate failed (" << ret << ")";
			throw ZlibError(oss.str());
		}
		have = buffer.size() - stream.avail_out;
		output.append(reinterpret_cast<char *>(buffer.data()), have);
	} while (ret != Z_STREAM_END);

	inflateEnd(&stream);
	return output;
}
