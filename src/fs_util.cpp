#include "fs_util.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace {
	bool is_invalid_chr(unsigned char ch)
	{
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
	std::ostringstream oss;
	std::ifstream input(path, std::ios::binary);
	std::error_code ec;
	std::uintmax_t size;
	std::vector<uint8_t> data;

	if (!input)
	{
		oss << "Failed to open input file: " << path;
		throw std::runtime_error(oss.str());
	}

	size = std::filesystem::file_size(path, ec);
	if (ec)
	{
		oss << "Failed to determine size of input file: " << path;
		throw std::runtime_error(oss.str());
	}

	data.resize(static_cast<size_t>(size));
	if (size > 0)
	{
		input.read(reinterpret_cast<char *>(data.data()),
			static_cast<std::streamsize>(size));
		if (!input)
		{
			oss << "Failed to read input file: " << path;
			throw std::runtime_error(oss.str());
		}
	}
	return data;
}

void write_file(const std::string &path, const std::string &data)
{
	std::ostringstream oss;
	std::ofstream output(path, std::ios::binary | std::ios::trunc);

	if (!output)
	{
		oss << "Failed to open output file: " << path;
		throw std::runtime_error(oss.str());
	}
	output.write(data.data(), static_cast<std::streamsize>(data.size()));
	if (!output)
	{
		oss << "Failed to write output file: " << path;
		throw std::runtime_error(oss.str());
	}
}

std::string sanitize_filename(const std::string &name)
{
	std::string cleaned;
	size_t begin;
	size_t end;

	begin = name.find_first_not_of(' ');
	end = name.find_last_not_of(" .");
	if (end == std::string::npos)
	{
		return "Untitled";
	}
	cleaned.reserve(end - begin + 1);
	for (unsigned char ch : name.substr(begin, end - begin + 1))
	{
		if (!is_invalid_chr(ch) && !isspace(ch))
		{
			cleaned.push_back(static_cast<char>(ch));
		}
		else if (isspace(ch))
		{
			cleaned.push_back('_');
		}
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
