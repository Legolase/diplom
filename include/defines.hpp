#ifndef _DEFINES_HPP
#define _DEFINES_HPP

#include <fmt/format.h>

#define CURRENT_POSITION \
  (fmt::format("{}:{} {}(...)", __FILE__, __LINE__, __FUNCTION__))

#endif