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
  using storage_t = std::string_view;

public:
  /// @brief Simple work with a given С-string without copying
  /// @param[in] source C-string line
  /// @param[in] size Size of C-string line
  StringBufferReader(const char* source, size_t size) noexcept;
  /// @brief Same as `StringBufferReader(const char* source)` but with wrapper
  /// `std::string_view`
  explicit StringBufferReader(const std::string_view& source) noexcept;

  StringBufferReader(const StringBufferReader&) = delete;

  StringBufferReader& operator=(const StringBufferReader&) = delete;

  /// @brief Read row data with advance to T value type and return it
  /// @tparam T is default constructible class
  /// @returns T value constructed from row binary data relative to current position
  /// @throws `BadStream` Thrown if `sizeof(T)` more than available to
  /// read
  template<typename T>
    requires std::is_default_constructible_v<T>
  T read()
  {
    T value;
    readCpy(reinterpret_cast<char*>(&value), sizeof(value));

    return value;
  }

  /// @brief Same as `read()` but without advance
  /// @tparam T is default constructible class
  /// @param[in] offset Unsigned offset to jump to before reading the stream
  /// @returns T value constructed from row binary data relative to current position with
  /// offset
  /// @throws `BadStream` Thrown if `offset + sizeof(T)` more than
  /// available to read
  template<typename T>
    requires std::is_default_constructible_v<T>
  T peek(size_t offset = 0)
  {
    T value;

    peekCpy(reinterpret_cast<char*>(&value), offset, sizeof(value));

    return value;
  }

  /// @brief Row copy of memory to specified place.
  /// @param[in, out] dest Place to copy sequence of character.
  /// @param[in] size Size of required sequence.
  void readCpy(char* dest, size_t size);

  /// @brief Row copy of memory relative to offset to specified place.
  /// @param[in, out] dest Place to copy sequence of character.
  /// @param[in] offset Unsigned offset to jump to before reading the stream.
  /// @param[in] size Size of required sequence.
  void peekCpy(char* dest, size_t offset, size_t size);

  /// @brief Tail size
  /// @return Size of left part of buffer
  size_t available() const noexcept;

  /// @brief Flips the available end of the buffer by the specified length of bytes.
  /// @param[in] length Number of bytes to flip.
  /// @throws `BadStream` Thrown if the length is invalid and exceeds the `available()`.
  void flipEnd(size_t length);

  /// @brief Returns current position relative to beginning of the buffer.
  /// @return Current position
  size_t position() const noexcept;

  /// @brief Returns a pointer to the memory next to be read.
  /// @return Pointer to the memory next to be read.
  const char* ptr() const noexcept;

  /// @brief Moves the read position back by the specified number of bytes.
  /// @param[in] length The number of bytes to move back relative to the current position.
  /// @throws `BadStream` Thrown if `length > position()`
  void rewind(size_t length);

  /// @brief Skips the next `length` bytes.
  /// @param[in] length The amount to jump forward relative to the current position.
  /// @throws `BadStream` Thrown if `length > available()`
  void skip(size_t length);

  /// @brief Resets the read position to the beginning of the buffer.
  /// This method sets the internal cursor to the start of the buffer,
  /// effectively allowing to re-read data from the beginning.
  void restart() noexcept;

private:
  /// @brief Buffer begining.
  /// @returns Pointer to buffer begining.
  const char* source() const noexcept;
  /// @brief Buffer size.
  size_t size() const noexcept;

  /// @brief union kept own string or string_view.
  storage_t storage_v;
  /// @brief current position of first unread character.
  size_t pos{0};
};
} // namespace utils

#endif