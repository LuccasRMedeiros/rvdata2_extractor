#include "fs_util.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
	bool is_path_hostile(unsigned char ch)
	{
		if (ch < 0x20)
		{
			return true;
		}
		switch (ch)
		{
			case '/':
			case '\\':
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
				return true;
			default:
				return false;
		}
	}
}

std::vector<uint8_t> read_file(const std::string &path)
{
	std::ifstream input(path, std::ios::binary);
	std::streamoff size;
	std::vector<uint8_t> data;

	if (!input)
	{
		std::ostringstream oss;

		oss << "Failed to open input file: " << path;
		throw std::runtime_error(oss.str());
	}

	input.seekg(0, std::ios::end);
	size = input.tellg();
	if (size < 0)
	{
		std::ostringstream oss;

		oss << "Failed to determine size of input file: " << path;
		throw std::runtime_error(oss.str());
	}
	input.seekg(0, std::ios::beg);

	data.resize(static_cast<size_t>(size));
	if (size > 0)
	{
		input.read(reinterpret_cast<char *>(data.data()), size);
		if (!input)
		{
			std::ostringstream oss;

			oss << "Failed to read input file: " << path;
			throw std::runtime_error(oss.str());
		}
	}
	return data;
}

void write_file(const std::string &path, const std::string &data)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);

	if (!output)
	{
		std::ostringstream oss;

		oss << "Failed to open output file: " << path;
		throw std::runtime_error(oss.str());
	}
	output.write(data.data(), static_cast<std::streamsize>(data.size()));
	if (!output)
	{
		std::ostringstream oss;

		oss << "Failed to write output file: " << path;
		throw std::runtime_error(oss.str());
	}
}

std::string sanitize_filename(const std::string &name)
{
	std::string cleaned;

	cleaned.reserve(name.size());
	for (unsigned char ch : name)
	{
		if (is_path_hostile(ch))
		{
			cleaned.push_back('_');
		}
		else
		{
			cleaned.push_back(static_cast<char>(ch));
		}
	}

	while (!cleaned.empty())
	{
		const char last = cleaned.back();

		if (last == ' ' || last == '.')
		{
			cleaned.pop_back();
			continue;
		}
		break;
	}

	if (cleaned.empty())
	{
		return "Untitled";
	}
	return cleaned;
}

std::string join_path(const std::string &dir, const std::string &name)
{
	if (dir.empty())
	{
		return name;
	}
	if (dir.back() == '/')
	{
		return dir + name;
	}
	return dir + "/" + name;
}
