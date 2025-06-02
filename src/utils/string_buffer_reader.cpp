#include <utils/string_buffer_reader.hpp>

namespace utils {

StringBufferReader::StringBufferReader(const char* source, size_t size) noexcept :
    storage_v(std::string_view(source, size))
{}

StringBufferReader::StringBufferReader(const std::string_view& source) noexcept :
    storage_v(source)
{}

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

  storage_v.remove_suffix(length);
}

size_t StringBufferReader::position() const noexcept
{
  return pos;
}

const char* StringBufferReader::ptr() const noexcept
{
  return source() + position();
}

void StringBufferReader::rewind(size_t length)
{
  if (length > position()) {
    THROW(BadStream, "Attempt to jump before the buffer start");
  }

  pos -= length;
}

void StringBufferReader::skip(size_t length)
{
  if (length > available()) {
    THROW(BadStream, "Attempt to jump beyond the buffer boundaries");
  }

  pos += length;
}

void StringBufferReader::restart() noexcept
{
  pos = 0;
}

const char* StringBufferReader::source() const noexcept
{
  return storage_v.data();
}

size_t StringBufferReader::size() const noexcept
{
  return storage_v.size();
}

} // namespace utils