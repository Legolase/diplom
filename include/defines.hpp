#ifndef _DEFINES_HPP
#define _DEFINES_HPP

#include <fmt/format.h>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <type_traits>
#include <vector>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define CURRENT_POSITION                                                                 \
  (fmt::format(": {}:{} {}(...): ", __FILENAME__, __LINE__, __FUNCTION__))

#define THROW(exception_type, message)                                                   \
  (throw (exception_type)(fmt::format(                                                   \
      "{}{}{}", #exception_type, CURRENT_POSITION, (message)                             \
  )))

#define DECLARE_EXCEPTION(class_name)                                                    \
  struct class_name final : std::runtime_error {                                         \
    template<typename... Args>                                                           \
    explicit class_name(Args&&... args) :                                                \
        std::runtime_error(std::forward<Args>(args)...)                                  \
    {}                                                                                   \
  }

namespace defines_details {

template<typename T>
  requires std::is_convertible_v<T, char> || std::is_convertible_v<T, unsigned char>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec)
{
  out << std::setfill('0');
  for (const auto& elem : vec) {
    out << "[" << std::setw(2) << std::hex << std::uppercase << static_cast<int>(elem)
        << "]";
  }
  out << std::dec << std::setw(0) << std::setfill(' ');
  return out;
}

struct LogStreamer;

LogStreamer log_info_impl(std::ostream& out = std::cout);
LogStreamer log_debug_impl(std::ostream& out = std::cout);
LogStreamer log_warning_impl(std::ostream& out = std::cout);
LogStreamer log_error_impl(std::ostream& out = std::cerr);

struct LogStreamer {
  explicit LogStreamer(std::ostream& out_) noexcept;
  ~LogStreamer();

  LogStreamer(LogStreamer&& other) noexcept;

  template<typename T>
  LogStreamer& operator<<(T&& value)
  {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
      return print(std::forward<T>(value));
    } else {
      out << std::forward<T>(value);
    }

    return *this;
  }

private:
  friend LogStreamer log_info_impl(std::ostream&);
  friend LogStreamer log_debug_impl(std::ostream&);
  friend LogStreamer log_warning_impl(std::ostream&);
  friend LogStreamer log_error_impl(std::ostream&);

  LogStreamer(LogStreamer& other) noexcept;

  LogStreamer& print(const std::string_view& str_view);

  std::ostream& out;
  bool holded{true};
};

struct NullLogStreamer {
  template<typename T>
  NullLogStreamer operator<<(T&& value)
  {
    return *this;
  }
};

static_assert(std::is_copy_assignable_v<LogStreamer> == false);
static_assert(std::is_copy_constructible_v<LogStreamer> == false);
static_assert(std::is_move_assignable_v<LogStreamer> == false);
static_assert(std::is_move_constructible_v<LogStreamer>);

} // namespace defines_details

#define LOG_INFO(...) (defines_details::log_info_impl(__VA_ARGS__))
#ifndef NDEBUG
#define LOG_DEBUG(...) (defines_details::log_debug_impl(__VA_ARGS__))
#else
#define LOG_DEBUG(...) (defines_details::NullLogStreamer{})
#endif
#define LOG_WARNING(...) (defines_details::log_warning_impl(__VA_ARGS__))
#define LOG_ERROR(...) (defines_details::log_error_impl(__VA_ARGS__))

#endif