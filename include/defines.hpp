#ifndef _DEFINES_HPP
#define _DEFINES_HPP

#include <iostream>
#include <fmt/format.h>

#define CURRENT_POSITION \
  (fmt::format("{}:{} {}(...)", __FILE__, __LINE__, __FUNCTION__))

#define LOG_INFO()    (std::cout << "   [INFO] ")
#define LOG_WARNING() (std::cout << "[WARNING] ")
#define LOG_ERROR()   (std::cout << "  [ERROR] ")

#endif