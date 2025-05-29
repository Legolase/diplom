#ifndef _UTILS_STREAM_READER_HPP
#define _UTILS_STREAM_READER_HPP

#include <defines.hpp>
#include <utils/common.hpp>

#include <iostream>
#include <type_traits>

namespace utils {

/**
 * @brief Class for convenient relative reading from a stream.
 *
 * This class wraps around a standard input stream and provides helper methods for reading
 * binary data relative to the current position. It simplifies the use of standard stream
 * methods when dealing with binary data.
 */
struct StreamReader {

  /**
   * @brief Constructor from the required stream for convenient reading of binary data
   * relative to the current position
   * @param[in] in The stream from which to read binary data
   */
  explicit StreamReader(std::istream& in) noexcept;

  /**
   * @brief Read row data with advance to T value type and return it
   * @tparam T is default constructible class
   * @returns T value constructed from row binary data relative to current position
   * @throws `BadStream` Thrown if `sizeof(T)` more than available to
   * read
   */
  template<typename T>
    requires std::is_default_constructible_v<T>
  T read()
  {
    T value;

    readCpy(reinterpret_cast<char*>(&value), sizeof(value));

    return value;
  }

  /**
   * @brief Same as `read()` but without advance
   * @tparam T is default constructible class
   * @param[in] offset Unsigned offset to jump to before reading the stream
   * @returns T value constructed from row binary data relative to current position with
   * offset
   * @throws `BadStream` Thrown if `offset + sizeof(T)` more than
   * available to read
   */
  template<typename T>
    requires std::is_default_constructible_v<T>
  T peek(size_t offset = 0)
  {
    T value;

    peekCpy(reinterpret_cast<char*>(&value), offset, sizeof(value));

    return value;
  }

  /**
   * @brief Row copy of memory to specified place
   * @param[in, out] dest Place to copy sequence of character
   * @param[in] size Size of required sequence
   * @throws `BadStream` Thrown if `size` more than available to read
   */
  void readCpy(char* dest, size_t size);

  /**
   * @brief Row copy of memory relative to offset to specified place
   * @param[in, out] dest Place to copy sequence of character
   * @param[in] offset Unsigned offset to jump to before reading the stream
   * @param[in] size Size of required sequence
   * @throws `BadStream` Thrown if `offset + size` more than available to read
   */
  void peekCpy(char* dest, size_t offset, size_t size);

  /**
   * @brief Check if the stream has reached its end.
   * @return true if end of stream, false otherwise
   */
  bool isEnd() noexcept;

private:
  /// @brief The stream from which to read binary data
  std::istream& stream;
};

} // namespace utils

#endif