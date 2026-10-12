#include "scripts_extract.hpp"

#include "fs_util.hpp"
#include "marshal.hpp"
#include "zlib_util.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
	const char SECTION_MARK[] = "\xE2\x96\xBC";
	const char TREE_FILENAME[] = "scripts.tree";

	struct TreeEntry {
		int64_t id;
		std::string file;
		std::string name;
	};

	std::string unique_name(const std::string &base, std::set<std::string> &used)
	{
		std::string candidate;
		size_t suffix;

		candidate = base;
		suffix = 2;
		while (used.count(candidate) != 0)
		{
			std::ostringstream oss;

			oss << base << "_" << suffix;
			candidate = oss.str();
			++suffix;
		}
		used.insert(candidate);
		return candidate;
	}

	std::string section_dirname(const std::string &name)
	{
		std::string stripped;
		size_t index;
		const size_t mark_len = sizeof(SECTION_MARK) - 1;

		stripped.reserve(name.size());
		index = 0;
		while (index < name.size())
		{
			if (name.compare(index, mark_len, SECTION_MARK) == 0)
			{
				index += mark_len;
				continue;
			}
			stripped.push_back(name[index]);
			++index;
		}
		return sanitize_filename(stripped);
	}

	std::string unescape_name(const std::string &text)
	{
		std::string out;

		out.reserve(text.size());
		for (size_t i = 0; i < text.size(); ++i)
		{
			if (text[i] == '\\' && i + 1 < text.size())
			{
				++i;
				switch (text[i])
				{
					case '\\':
						out.push_back('\\');
						break;
					case 't':
						out.push_back('\t');
						break;
					case 'n':
						out.push_back('\n');
						break;
					case 'r':
						out.push_back('\r');
						break;
					default:
						out.push_back(text[i]);
						break;
				}
				continue;
			}
			out.push_back(text[i]);
		}
		return out;
	}

	std::string format_tree_line(int64_t id, const std::string &file, const std::string &name)
	{
		std::string out;

		out = std::to_string(id);
		out.push_back('\t');
		out += file;
		out.push_back('\t');
		for (char ch : name)
		{
			switch (ch)
			{
				case '\\':
					out += "\\\\";
					break;
				case '\t':
					out += "\\t";
					break;
				case '\n':
					out += "\\n";
					break;
				case '\r':
					out += "\\r";
					break;
				default:
					out.push_back(ch);
					break;
			}
		}
		out.push_back('\n');
		return out;
	}

	TreeEntry parse_tree_line(const std::string &line, size_t line_no)
	{
		TreeEntry entry;
		size_t tab1;
		size_t tab2;
		std::ostringstream oss;

		tab1 = line.find('\t');
		if (tab1 == std::string::npos)
		{
			oss << "scripts.tree line " << line_no << " is missing a tab";
			throw std::runtime_error(oss.str());
		}
		tab2 = line.find('\t', tab1 + 1);
		if (tab2 == std::string::npos)
		{
			oss << "scripts.tree line " << line_no << " is missing a second tab";
			throw std::runtime_error(oss.str());
		}

		try
		{
			entry.id = std::stoll(line.substr(0, tab1));
		}
		catch (const std::exception &)
		{
			oss << "scripts.tree line " << line_no << " has an invalid id";
			throw std::runtime_error(oss.str());
		}
		entry.file = line.substr(tab1 + 1, tab2 - tab1 - 1);
		entry.name = unescape_name(line.substr(tab2 + 1));
		return entry;
	}

	void mount_tree(const std::string &text, std::vector<TreeEntry> &entries, std::map<int64_t, size_t> &by_id)
	{
		size_t begin;
		size_t line_no;

		begin = 0;
		line_no = 0;
		while (begin <= text.size())
		{
			size_t end;
			std::string line;

			end = text.find('\n', begin);
			if (end == std::string::npos)
			{
				line = text.substr(begin);
				begin = text.size() + 1;
			}
			else
			{
				line = text.substr(begin, end - begin);
				begin = end + 1;
			}
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			if (line.empty())
			{
				continue;
			}
			++line_no;
			{
				TreeEntry entry;
				std::ostringstream oss;

				entry = parse_tree_line(line, line_no);
				if (by_id.count(entry.id) != 0)
				{
					oss << "scripts.tree has duplicate id " << entry.id;
					throw std::runtime_error(oss.str());
				}
				by_id[entry.id] = entries.size();
				entries.push_back(entry);
			}
		}
	}

	void seed_reserved(const std::vector<TreeEntry> &entries,
		std::map<std::string, std::set<std::string>> &used_files,
		std::set<std::string> &used_dirs)
	{
		for (const TreeEntry &entry : entries)
		{
			size_t slash;
			std::string dir;
			std::string base;

			if (entry.file == "-" || entry.file.empty())
			{
				continue;
			}
			slash = entry.file.rfind('/');
			if (slash == std::string::npos)
			{
				dir = "";
				base = entry.file;
			}
			else
			{
				dir = entry.file.substr(0, slash);
				base = entry.file.substr(slash + 1);
				used_dirs.insert(dir);
			}
			if (base.size() >= 3 && base.compare(base.size() - 3, 3, ".rb") == 0)
			{
				base = base.substr(0, base.size() - 3);
			}
			used_files[dir].insert(base);
		}
	}

	std::string section_dir_from_tree(const std::vector<TreeEntry> &entries, size_t section_index)
	{
		size_t i;

		for (i = section_index + 1; i < entries.size(); ++i)
		{
			size_t slash;

			if (entries[i].name.find(SECTION_MARK) != std::string::npos)
			{
				break;
			}
			if (entries[i].file == "-" || entries[i].file.empty())
			{
				continue;
			}
			slash = entries[i].file.rfind('/');
			if (slash == std::string::npos)
			{
				return "";
			}
			return entries[i].file.substr(0, slash);
		}
		return "";
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

	void ensure_parent(const std::string &output_dir, const std::string &rel)
	{
		size_t slash;

		slash = rel.rfind('/');
		if (slash == std::string::npos)
		{
			return;
		}
		ensure_directory(join_path(output_dir, rel.substr(0, slash)));
	}
}

size_t scripts_extract(const std::string &input_path, const std::string &output_dir)
{
	const std::vector<uint8_t> data = read_file(input_path);
	const marshal::Value root = marshal::load(data);
	std::vector<TreeEntry> mounted;
	std::map<int64_t, size_t> by_id;
	std::map<std::string, std::set<std::string>> used_files;
	std::set<std::string> used_dirs;
	std::vector<TreeEntry> result;
	std::string current_dir;
	std::string tree_path;
	std::string tree_text;
	size_t written = 0;

	if (root.type != marshal::Type::Array)
	{
		throw std::runtime_error("Scripts.rvdata2 root object is not an array");
	}

	ensure_directory(output_dir);
	tree_path = join_path(output_dir, TREE_FILENAME);
	{
		struct stat st;

		if (stat(tree_path.c_str(), &st) != 0)
		{
			write_file(tree_path, "");
		}
	}
	{
		const std::vector<uint8_t> tree_bytes = read_file(tree_path);

		tree_text.assign(reinterpret_cast<const char *>(tree_bytes.data()), tree_bytes.size());
	}
	mount_tree(tree_text, mounted, by_id);
	seed_reserved(mounted, used_files, used_dirs);

	{
		const std::vector<marshal::Value> &entries = root.as_array();

		result.reserve(entries.size());
		for (size_t i = 0; i < entries.size(); ++i)
		{
			const marshal::Value &entry = entries[i];
			int64_t id;
			std::string name;
			std::string compressed;
			std::string source;
			std::string rel;
			std::map<int64_t, size_t>::const_iterator known;
			bool is_section;

			if (entry.type != marshal::Type::Array)
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

				if (fields[0].type != marshal::Type::Integer)
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " id is not an integer";
					throw std::runtime_error(oss.str());
				}

				if (fields[1].type != marshal::Type::String)
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " name is not a string";
					throw std::runtime_error(oss.str());
				}

				if (fields[2].type != marshal::Type::String)
				{
					std::ostringstream oss;

					oss << "Script entry " << (i + 1) << " body is not a string";
					throw std::runtime_error(oss.str());
				}

				id = fields[0].as_integer();
				name = fields[1].as_string();
				compressed = fields[2].as_string();
			}

			source = zlib_inflate(compressed);
			known = by_id.find(id);
			is_section = name.find(SECTION_MARK) != std::string::npos;

			if (is_section)
			{
				if (known != by_id.end())
				{
					current_dir = section_dir_from_tree(mounted, known->second);
					if (current_dir.empty())
					{
						current_dir = unique_name(section_dirname(name), used_dirs);
					}
				}
				else
				{
					current_dir = unique_name(section_dirname(name), used_dirs);
				}
				ensure_directory(join_path(output_dir, current_dir));
				if (!source.empty())
				{
					std::cerr << "warning: section \"" << name << "\" (id " << id
						<< ") has source text; ignoring it" << std::endl;
				}
				result.push_back(TreeEntry{id, "-", name});
				continue;
			}

			if (source.empty())
			{
				result.push_back(TreeEntry{id, "-", name});
				continue;
			}

			if (known != by_id.end() && mounted[known->second].file != "-")
			{
				rel = mounted[known->second].file;
			}
			else
			{
				std::string stem;

				stem = unique_name(sanitize_filename(name), used_files[current_dir]);
				if (current_dir.empty())
				{
					rel = stem + ".rb";
				}
				else
				{
					rel = join_path(current_dir, stem + ".rb");
				}
			}
			ensure_parent(output_dir, rel);
			write_file(join_path(output_dir, rel), source);
			result.push_back(TreeEntry{id, rel, name});
			++written;
		}
	}

	{
		std::string out;

		for (const TreeEntry &entry : result)
		{
			out += format_tree_line(entry.id, entry.file, entry.name);
		}
		write_file(tree_path, out);
	}
	return written;
}
