#ifndef _BIT_BUFFER_READER_HPP
#define _BIT_BUFFER_READER_HPP

#include <defines.hpp>
#include <utils/common.hpp>

#include <stdexcept>
#include <string>
#include <variant>

namespace utils {

using bit = bool;

/// Enum for defining bit read order within a byte
enum class BitOrder {
  LITTLE_END, ///< Read from least significant bit to most
  BIG_END     ///< Read from most significant bit to least
};

/**
 * @brief Class for sequential bit reading from a memory buffer.
 *
 * This template class allows reading bits from a given string or string_view.
 * Depending on the BitOrder, the bit order within each byte can be reversed.
 *
 * @tparam Order The bit reading order (BitOrder::LITTLE_END or BitOrder::BIG_END)
 */
template<BitOrder Order>
class BitBufferReader {
  using storage_t = std::string_view;

public:
  /**
   * @brief Construct a BitBufferReader from a raw character buffer without copy semantic.
   * Ownership of the original buffer remains with the calling function. Overwriting the
   * original buffer results in undefined behavior.
   *
   * @param source Pointer to the input buffer
   * @param size Length of the buffer in bytes
   */
  BitBufferReader(const char* source, size_t size) noexcept :
      storage_v(std::string_view(source, size))
  {}

  /**
   * @brief Construct a BitBufferReader from a string view. Convention same as in
   * `BitBufferReader(const char* source, size_t size)`.
   *
   * @see BitBufferReader(const char* source, size_t size)
   *
   * @param source A view into the input data
   */
  explicit BitBufferReader(const std::string_view& source) noexcept :
      storage_v(source)
  {}

  BitBufferReader(const BitBufferReader&) = delete;

  BitBufferReader& operator=(const BitBufferReader&) = delete;

  /**
   * @brief Peek the next bit without advancing the reader.
   * @return The next bit in the buffer
   * @throws std::runtime_error if no bits are available
   */
  bit peek()
  {
    if (!available()) {
      THROW(std::runtime_error, "Unavailable to read more bits. End of stream.");
    }

    uint8_t current_char = storage_v[pos / 8];
    uint8_t real_byte_pos = pos % 8;

    if constexpr (Order == BitOrder::BIG_END) {
      real_byte_pos = 7 - real_byte_pos;
    }

    return (current_char >> real_byte_pos) & 1;
  }
  /**
   * @brief Read the next bit and advance the reader.
   * @return The next bit in the buffer
   * @throws std::runtime_error if no bits are available
   */
  bit read()
  {
    auto result = peek();
    ++pos;
    return result;
  }

  /// @brief Resets the read position to the beginning of the bit stream.
  void restart() noexcept
  {
    pos = 0;
  }

private:
  /**
   * @brief Get the total size of the buffer in bits.
   * @return Number of bits in the input buffer
   */
  size_t size() const noexcept
  {
    return storage_v.size() * 8;
  }

  /**
   * @brief Get the number of bits still available for reading.
   * @return Number of bits remaining
   */
  size_t available() const noexcept
  {
    return size() - pos;
  }

  /// Internal storage: either owned string or non-owning string_view
  storage_t storage_v;
  /// Current reading position in bits
  size_t pos{0};
};

} // namespace utils

#endif