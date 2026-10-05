#include "scripts_extract.hpp"

#include "fs_util.hpp"
#include "marshal.hpp"
#include "zlib_util.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
	std::string format_index(size_t index)
	{
		std::ostringstream oss;

		oss << std::setw(4) << std::setfill('0') << index;
		return oss.str();
	}

	std::string script_filename(size_t index, const std::string &name)
	{
		const std::string safe_name = sanitize_filename(name);

		return format_index(index) + "_" + safe_name + ".rb";
	}

	void ensure_directory(const std::string &path)
	{
		struct stat st;
		std::ostringstream oss;

		if (stat(path.c_str(), &st) == 0)
		{
			if (!S_ISDIR(st.st_mode))
			{
				oss << "Output path exists and is not a directory: " << path;
				throw std::runtime_error(oss.str());
			}
			return;
		}

		if (mkdir(path.c_str(), 0755) != 0)
		{
			if (errno == EEXIST)
			{
				return;
			}
			oss << "Failed to create directory " << path << ": " << std::strerror(errno);
			throw std::runtime_error(oss.str());
		}
	}
}

size_t scripts_extract(const std::string &input_path, const std::string &output_dir)
{
	const std::vector<uint8_t> data = read_file(input_path);
	const marshal::Value root = marshal::load(data);
	size_t written = 0;

	if (!root.is_array())
	{
		throw std::runtime_error("Scripts.rvdata2 root object is not an array");
	}

	ensure_directory(output_dir);

	{
		const std::vector<marshal::Value> &entries = root.as_array();

		for (size_t i = 0; i < entries.size(); ++i)
		{
			const marshal::Value &entry = entries[i];
			std::string name;
			std::string compressed;
			std::string source;
			std::string filename;
			std::string path;

			if (!entry.is_array())
			{
				std::ostringstream oss;

				oss << "Script entry " << (i + 1) << " is not an array";
				throw std::runtime_error(oss.str());
			}

			{
				const std::vector<marshal::Value> &fields = entry.as_array();

				if (fields.size() < 3)
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " has fewer than 3 fields";
					throw std::runtime_error(oss.str());
				}

				if (!fields[0].is_integer())
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " id is not an integer";
					throw std::runtime_error(oss.str());
				}

				if (!fields[1].is_string())
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " name is not a string";
					throw std::runtime_error(oss.str());
				}

				if (!fields[2].is_string())
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " body is not a string";
					throw std::runtime_error(oss.str());
				}

				name = fields[1].as_string();
				compressed = fields[2].as_string();
			}

			source = zlib_inflate(compressed);
			if (source.empty())
			{
				continue;
			}

			filename = script_filename(i + 1, name);
			path = join_path(output_dir, filename);
			write_file(path, source);
			++written;
		}
	}

	return written;
}
