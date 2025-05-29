#ifndef _BIT_BUFFER_READER_HPP
#define _BIT_BUFFER_READER_HPP

#include <defines.hpp>
#include <utils/common.hpp>

#include <stdexcept>
#include <string>
#include <variant>

namespace utils {

using bit = bool;

enum class BitOrder {
  LITTLE_END,
  BIG_END
};

/**
 * The class is used to sequentially read bits of some memory area. Depending on the
 * BitOrder specified, reading can be reversed within a byte.
 */
template<BitOrder Order>
class BitBufferReader {
  using storage_t = std::variant<std::string, std::string_view>;

public:
  explicit BitBufferReader(std::string line) :
      storage_v(std::move(line))
  {}

  BitBufferReader(const char* source, size_t size) noexcept :
      storage_v(std::string_view(source, size))
  {}

  explicit BitBufferReader(const std::string_view& source) noexcept :
      storage_v(source)
  {}

  BitBufferReader(const BitBufferReader&) = delete;

  BitBufferReader& operator=(const BitBufferReader&) = delete;

  bit peek()
  {
    if (!available()) {
      THROW(std::runtime_error, "Unavailable to read more bits. End of stream.");
    }

    uint8_t current_char = std::visit(
        Overload{ [this](const std::string& str) {
                    return str[pos / 8];
                  },
                  [this](const std::string_view& str_view) {
                    return str_view[pos / 8];
                  } },
        storage_v
    );
    uint8_t real_byte_pos = pos % 8;

    if constexpr (Order == BitOrder::BIG_END) {
      real_byte_pos = 7 - real_byte_pos;
    }

    return (current_char >> real_byte_pos) & 1;
  }

  bit read()
  {
    auto result = peek();
    ++pos;
    return result;
  }

private:
  size_t size() const noexcept
  {
    return std::visit(
        Overload{ [](const std::string& str) {
                   return str.size() * 8;
                 },
                  [](const std::string_view& str_view) {
                    return str_view.size() * 8;
                  } },
        storage_v
    );
  }

  size_t available() const noexcept
  {
    return size() - pos;
  }

  storage_t storage_v;
  /// @brief current reading bit. It's storage_v[pos / 8][pos % 8]. pos % 8 -- depends on
  /// BitOrder
  size_t pos{ 0 };
};

} // namespace utils

#endif