#include <utils/stream_reader.hpp>

namespace utils {
StreamReader::StreamReader(std::istream& in) noexcept :
    stream(in) {}

void StreamReader::readCpy(char* dest, size_t size) {
  const auto read_size = stream.readsome(dest, size);

  if (size != read_size) {
    throw BadStream(fmt::format("[Error] Attempt to read more than possible in {}",
                                CURRENT_POSITION));
  }
}

void StreamReader::peekCpy(char* dest, size_t offset, size_t size) {
  const auto start_pos = stream.tellg();

  stream.seekg(offset, std::ios::cur);
  readCpy(dest, size);
  stream.seekg(start_pos);
}
} // namespace utils