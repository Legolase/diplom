#include <utils/string_buffer_reader.hpp>

namespace utils {

StringBufferReader::StringBufferReader(std::string line) :
    storage_v(std::move(line))
{
}

StringBufferReader::StringBufferReader(const char* source, size_t size) noexcept :
    storage_v(std::string_view(source, size))
{
}

StringBufferReader::StringBufferReader(const std::string_view& source) noexcept :
    storage_v(source)
{
}

void StringBufferReader::readCpy(char* dest, size_t size)
{
  if (size > available()) {
    THROW(BadStream, "Attempt to read more than left");
  }

  std::memcpy(dest, source() + pos, size);
  pos += size;
}

void StringBufferReader::peekCpy(char* dest, size_t offset, size_t size)
{
  const auto start_pos = pos;

  pos += offset;
  readCpy(dest, size);
  pos = start_pos;
}

size_t StringBufferReader::available() const noexcept
{
  if (pos >= size()) {
    return 0;
  }
  return size() - pos;
}

void StringBufferReader::flipEnd(size_t length)
{
  if (available() < length) {
    THROW(BadStream, "Attempt to flip more bytes than available");
  }

  std::visit(
      Overload{
          [&](std::string& str) {
            str.resize(str.size() - length);
          },
          [&](std::string_view& str_view) {
            str_view.remove_suffix(length);
          },
      },
      storage_v
  );
}

size_t StringBufferReader::position() const noexcept
{
  return pos;
}

void StringBufferReader::skip(size_t length)
{
  if (length > available()) {
    THROW(BadStream, "Attempt to jump beyond the buffer boundaries");
  }

  pos += length;
}

const char* StringBufferReader::source() const noexcept
{
  return std::visit(
      Overload{
          [](const std::string& str) {
            return str.data();
          },
          [](const std::string_view& str_view) {
            return str_view.data();
          }
      },
      storage_v
  );
}

size_t StringBufferReader::size() const noexcept
{
  return std::visit(
      Overload{
          [](const std::string& str) {
            return str.size();
          },
          [](const std::string_view& str_view) {
            return str_view.size();
          }
      },
      storage_v
  );
}

} // namespace utils