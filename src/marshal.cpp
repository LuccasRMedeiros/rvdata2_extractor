#include "marshal.hpp"

#include <sstream>

namespace {
	class Reader {
		public:
			explicit Reader(const std::vector<uint8_t> &data)
				: data_(data), pos_(0)
			{
			}

			marshal::Value load(void)
			{
				uint8_t major;
				uint8_t minor;

				if (data_.size() < 2)
				{
					throw marshal::Error("Marshal data too short for version header");
				}

				major = read_byte();
				minor = read_byte();
				if (major != 4 || minor != 8)
				{
					std::ostringstream oss;

					oss << "Unsupported Marshal version "
					    << static_cast<unsigned int>(major) << "."
					    << static_cast<unsigned int>(minor)
					    << " (expected 4.8)";
					throw marshal::Error(oss.str());
				}

				return read_value();
			}

		private:
			const std::vector<uint8_t> &data_;
			size_t pos_;
			std::vector<marshal::Value> objects_;
			std::vector<std::string> symbols_;

			uint8_t read_byte(void)
			{
				if (pos_ >= data_.size())
				{
					throw marshal::Error("Unexpected end of Marshal data");
				}
				return data_[pos_++];
			}

			std::vector<uint8_t> read_bytes(size_t count)
			{
				std::vector<uint8_t> out;

				if (pos_ + count > data_.size())
				{
					throw marshal::Error("Unexpected end of Marshal data");
				}
				out.assign(data_.begin() + static_cast<std::ptrdiff_t>(pos_),
					   data_.begin() + static_cast<std::ptrdiff_t>(pos_ + count));
				pos_ += count;
				return out;
			}

			int64_t read_long(void)
			{
				const int8_t first = static_cast<int8_t>(read_byte());

				if (first == 0)
				{
					return 0;
				}
				if (first > 0 && first < 5)
				{
					int64_t value = 0;

					for (int i = 0; i < first; ++i)
					{
						value |= static_cast<int64_t>(read_byte()) << (8 * i);
					}
					return value;
				}
				if (first > -5 && first < 0)
				{
					int64_t value = -1;

					for (int i = 0; i < -first; ++i)
					{
						value &= ~(static_cast<int64_t>(0xff) << (8 * i));
						value |= static_cast<int64_t>(read_byte()) << (8 * i);
					}
					return value;
				}
				if (first > 0)
				{
					return static_cast<int64_t>(first) - 5;
				}
				return static_cast<int64_t>(first) + 5;
			}

			std::string read_bytestring(void)
			{
				const int64_t length = read_long();
				std::vector<uint8_t> bytes;

				if (length < 0)
				{
					throw marshal::Error("Negative byte sequence length");
				}
				bytes = read_bytes(static_cast<size_t>(length));
				return std::string(bytes.begin(), bytes.end());
			}

			marshal::Value read_value(void)
			{
				const uint8_t tag = read_byte();

				switch (tag)
				{
					case '0':
						return marshal::Value::make_nil();
					case 'T':
						return marshal::Value::make_bool(true);
					case 'F':
						return marshal::Value::make_bool(false);
					case 'i':
						return marshal::Value::make_integer(read_long());
					case '"':
					{
						marshal::Value value = marshal::Value::make_string(read_bytestring());

						objects_.push_back(value);
						return value;
					}
					case ':':
					{
						std::string symbol = read_bytestring();

						symbols_.push_back(symbol);
						return marshal::Value::make_symbol(symbol);
					}
					case ';':
					{
						const int64_t index = read_long();

						if (index < 0 || static_cast<size_t>(index) >= symbols_.size())
						{
							throw marshal::Error("Invalid Marshal symbol link");
						}
						return marshal::Value::make_symbol(symbols_[static_cast<size_t>(index)]);
					}
					case '@':
					{
						const int64_t index = read_long();

						if (index < 0 || static_cast<size_t>(index) >= objects_.size())
						{
							throw marshal::Error("Invalid Marshal object link");
						}
						return objects_[static_cast<size_t>(index)];
					}
					case '[':
					{
						const int64_t length = read_long();
						marshal::Value value;
						size_t object_index;
						std::vector<marshal::Value> items;
						int64_t i;

						if (length < 0)
						{
							throw marshal::Error("Negative Marshal array length");
						}
						value = marshal::Value::make_array({});
						objects_.push_back(value);
						object_index = objects_.size() - 1;
						items.reserve(static_cast<size_t>(length));
						for (i = 0; i < length; ++i)
						{
							items.push_back(read_value());
						}
						value = marshal::Value::make_array(items);
						objects_[object_index] = value;
						return value;
					}
					case 'I':
					{
						marshal::Value value = read_value();
						const int64_t ivar_count = read_long();
						int64_t i;

						if (ivar_count < 0)
						{
							throw marshal::Error("Negative Marshal IVAR count");
						}
						for (i = 0; i < ivar_count; ++i)
						{
							(void)read_value(); /* symbol name */
							(void)read_value(); /* symbol value */
						}
						return value;
					}
					default:
					{
						std::ostringstream oss;

						oss << "Unsupported Marshal type tag 0x"
						    << std::hex << static_cast<unsigned int>(tag);
						throw marshal::Error(oss.str());
					}
				}
			}
	};
}

marshal::Error::Error(const std::string &message)
	: std::runtime_error(message)
{
}

marshal::Value::Value(void)
	: type(Type::Nil), bool_value(false), int_value(0)
{
}

marshal::Value marshal::Value::make_nil(void)
{
	return Value();
}

marshal::Value marshal::Value::make_bool(bool value)
{
	Value out;

	out.type = Type::Bool;
	out.bool_value = value;
	return out;
}

marshal::Value marshal::Value::make_integer(int64_t value)
{
	Value out;

	out.type = Type::Integer;
	out.int_value = value;
	return out;
}

marshal::Value marshal::Value::make_string(const std::string &value)
{
	Value out;

	out.type = Type::String;
	out.string_value = value;
	return out;
}

marshal::Value marshal::Value::make_symbol(const std::string &value)
{
	Value out;

	out.type = Type::Symbol;
	out.string_value = value;
	return out;
}

marshal::Value marshal::Value::make_array(const std::vector<Value> &value)
{
	Value out;

	out.type = Type::Array;
	out.array_value = value;
	return out;
}

int64_t marshal::Value::as_integer(void) const
{
	if (type != Type::Integer)
	{
		throw Error("Marshal value is not an integer");
	}
	return int_value;
}

const std::string &marshal::Value::as_string(void) const
{
	if (type != Type::String && type != Type::Symbol)
	{
		throw Error("Marshal value is not a string");
	}
	return string_value;
}

const std::vector<marshal::Value> &marshal::Value::as_array(void) const
{
	if (type != Type::Array)
	{
		throw Error("Marshal value is not an array");
	}
	return array_value;
}

marshal::Value marshal::load(const std::vector<uint8_t> &data)
{
	Reader reader(data);

	return reader.load();
}
