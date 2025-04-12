#ifndef _UTILS_STRING_BUFFER_READER_HPP
#define _UTILS_STRING_BUFFER_READER_HPP

#include <defines.hpp>
#include <utils/common.hpp>

#include <cstring>
#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace utils {

/**
 * The class is created specifically for convenient work with a limited allocated part of
 * the received stream in the form of a buffer. In the context of the project, it is used
 * specifically for the indivisible part of work with one event, when we need to know its
 * beginning and end for convenient division of its fields.
 * All methods with prefix `read` advance current position
 * of reading.
 * The other methods with prefix `fetch` doesn't move
 * current position. They just read relative to current
 * position.
 */
class StringBufferReader {
  using storage_t = std::variant<std::string, std::string_view>;

public:
  /**
   * Creates a stream instance with a copy of the source string. Useful when you don't
   * have permission to manage the allocated memory.
   *
   * @param[in] line Used to copy the source string. Forced `move(...)`, so the parameter
   * is clean. When the line is rvalue it is not copied
   */
  explicit StringBufferReader(std::string line);

  /// @brief Simple work with a given С-string without copying
  /// @param[in] source C-string line
  /// @param[in] size Size of C-string line
  explicit StringBufferReader(const char* source, size_t size) noexcept;
  /// @brief Same as `StringBufferReader(const char* source)` but with wrapper
  /// `std::string_view`
  explicit StringBufferReader(const std::string_view& source) noexcept;

  /// @brief Read row data with advance to T value type and return it
  /// @tparam T is default constructible class
  /// @returns T value constructed from row binary data relative to current position
  /// @throws `BadStream` Thrown if `sizeof(T)` more than available to
  /// read
  template <typename T>
    requires std::is_default_constructible_v<T>
  T read() {
    const auto avail = available();
    if (sizeof(T) > avail) {
      throw BadStream(
          fmt::format("[Error] Attempt to read more than left in {}", CURRENT_POSITION));
    }

    T value;
    char* dst = reinterpret_cast<char*>(&value);
    const char* src = source() + pos;

    std::memcpy(dst, src, sizeof(T));
    pos += sizeof(T);

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
    const auto start_pos = pos;

    pos += offset;
    value = read<T>();
    pos = start_pos;

    return value;
  }

  /// @brief Row copy of memory to specified place
  /// @param[in, out] dest Place to copy sequence of character
  /// @param[in] size Size of required sequence
  void readCpy(char* dest, size_t size);

  /// @brief Row copy of memory relative to offset to specified place
  /// @param[in, out] dest Place to copy sequence of character
  /// @param[in] offset Unsigned offset to jump to before reading the stream
  /// @param[in] size Size of required sequence
  void fetchCpy(char* dest, size_t offset, size_t size);

private:
  /// @brief Tail size
  /// @return Size of left part of buffer
  size_t available() const noexcept;
  /// @brief Buffer begining
  /// @returns Pointer to buffer begining
  const char* source() const noexcept;
  /// @brief Buffer size
  size_t size() const noexcept;

  /// @brief union kept own string or string_view
  storage_t storage_v;
  /// @brief current position of first unread character
  size_t pos{0};
};
} // namespace utils

#endif