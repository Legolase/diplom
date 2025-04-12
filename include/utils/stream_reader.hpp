#ifndef _UTILS_STREAM_READER_HPP
#define _UTILS_STREAM_READER_HPP

#include <defines.hpp>
#include <utils/common.hpp>

#include <iostream>
#include <type_traits>

namespace utils {

/**
 * This class is designed to conveniently work with a potentially infinite stream, where
 * we often need to read relative to the current position. The standard library provides
 * the necessary tools for this, but they are often not concise when used.
 */
struct StreamReader {
  /// @brief Constructor from the required stream for convenient reading of binary data
  /// relative to the current position
  /// @param[in] in The stream from which to read binary data
  explicit StreamReader(std::istream& in) noexcept;

  /// @brief Read row data with advance to T value type and return it
  /// @tparam T is default constructible class
  /// @returns T value constructed from row binary data relative to current position
  /// @throws `BadStream` Thrown if `sizeof(T)` more than available to
  /// read
  template <typename T>
    requires std::is_default_constructible_v<T>
  T read() {
    T value;

    const auto read_size = stream.readsome(reinterpret_cast<char*>(&value), sizeof(T));

    if (read_size != sizeof(T)) {
      throw BadStream(fmt::format("[Error] Attempt to read {} byte(s) in {}", sizeof(T),
                                  CURRENT_POSITION));
    }

    return value;
  }

  /// @brief Same as `read()` but without advance
  /// @tparam T is default constructible class
  /// @param[in] offset Unsigned offset to jump to before reading the stream
  /// @returns T value constructed from row binary data relative to current position with
  /// offset
  /// @throws `BadStream` Thrown if `offset + sizeof(T)` more than
  /// available to read
  template <typename T>
    requires std::is_default_constructible_v<T>
  T fetch(size_t offset = 0) {
    T value;
    const auto start_pos = stream.tellg();

    stream.seekg(offset, std::ios::cur);
    value = read<T>();
    stream.seekg(start_pos);

    return value;
  }

private:
  /// @brief The stream from which to read binary data
  std::istream& stream;
};

} // namespace utils

#endif