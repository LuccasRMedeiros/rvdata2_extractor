#ifndef __MARSHAL_HPP__
#define __MARSHAL_HPP__

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace marshal {
	enum class Type {
		Nil,
		Bool,
		Integer,
		String,
		Symbol,
		Array
	};

	class Error : public std::runtime_error {
		public:
			explicit Error(const std::string &message);
	};

	class Value {
		public:
			Type type;

			bool bool_value;
			int64_t int_value;
			std::string string_value;
			std::vector<Value> array_value;

			Value(void);
			static Value make_nil(void);
			static Value make_bool(bool value);
			static Value make_integer(int64_t value);
			static Value make_string(const std::string &value);
			static Value make_symbol(const std::string &value);
			static Value make_array(const std::vector<Value> &value);

			int64_t as_integer(void) const;
			const std::string &as_string(void) const;
			const std::vector<Value> &as_array(void) const;
	};

	Value load(const std::vector<uint8_t> &data);
}

#endif // __MARSHAL_HPP__
