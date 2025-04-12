#ifndef _UTILS_COMMON_HPP
#define _UTILS_COMMON_HPP

#include <stdexcept>

namespace utils {

/**
 * @brief Structure for throwing exceptions during normal
 * reading.
 */
struct BadStream : std::runtime_error {
  BadStream(const std::string& _what);
};

/// @brief Helper struct to use it in `std::visit`
/// @tparam ...Types
template <typename... Types>
struct Overload : Types... {
  using Types::operator()...;
};

} // namespace utils

#endif