#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <istream>
#include <stdexcept>
#include <defines.hpp>

namespace stream_utils {

struct bad_stream : std::runtime_error {
  bad_stream(const std::string& _what);
};

template<typename T>
T read(std::istream& in) {
  T value;
  const auto read_result = in.readsome(reinterpret_cast<char*>(value), sizeof(value));

  if (read_result != 0) {
    throw bad_stream(fmt::format("[Error] bad_stream exception in {}", CURRENT_POSITION));
  }

  return value;
}

template<typename T>
T fetch(std::istream& in, int offset = 0) {
  T value;
  const auto start_position = in.tellg();

  in.seekg(offset, std::ios::cur);

  value = read<T>(value, in);

  in.seekg(start_position);

  return value;
}

void memcpy(std::istream& in, char* dst, size_t size);

}

#endif