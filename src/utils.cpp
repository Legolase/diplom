#include <utils.hpp>

#include <cstring>
#include <memory>

namespace stream_utils {

bad_stream::bad_stream(const std::string& _what) :
    std::runtime_error(_what) {}

void memcpy(std::istream& in, char* dst, size_t size) {
  const auto read_result = in.readsome(dst, size);

  if (read_result != size) {
    throw bad_stream(fmt::format("[Error] bad_stream exception in {}", CURRENT_POSITION));
  }
}

} // namespace stream_utils
