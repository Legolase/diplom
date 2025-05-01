#include <defines.hpp>
#include <iomanip>

namespace defines_details {

LogStreamer::LogStreamer(std::ostream& out_) noexcept :
    out(out_) {}

LogStreamer::~LogStreamer() {
  if (holded) {
    out << std::endl;
  }
}

LogStreamer::LogStreamer(LogStreamer& other) noexcept :
    out(other.out) {
  other.holded = false;
}

LogStreamer::LogStreamer(LogStreamer&& other) noexcept :
    out(other.out) {
  other.holded = false;
}

LogStreamer& LogStreamer::print(const std::string_view& str_view) {
  out << std::setfill('0');
  for (auto elem : str_view) {
    out << "[" << std::setw(2) << std::hex << std::uppercase
        << static_cast<int>(*reinterpret_cast<unsigned char*>(&elem)) << "]";
  }
  out << std::dec << std::setw(0) << std::setfill(' ');
  return *this;
}

LogStreamer log_info_impl(std::ostream& out) {
  LogStreamer streamer(out);
  return streamer << "   [INFO] ";
}

LogStreamer log_warning_impl(std::ostream& out) {
  LogStreamer streamer(out);
  return streamer << "[WARNING] ";
}

LogStreamer log_error_impl(std::ostream& out) {
  LogStreamer streamer(out);
  return streamer << "  [ERROR] ";
}

} // namespace defines_details