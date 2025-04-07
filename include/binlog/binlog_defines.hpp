#ifndef _BINLOG_DEFINES_HPP
#define _BINLOG_DEFINES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace mysql_binlog {
inline constexpr size_t ST_SERVER_VER_LEN = 50;
inline constexpr size_t ST_SERVER_VER_SPLIT_LEN = 3;
inline constexpr size_t MAX_DBS_IN_EVENT_MTS = 16;
inline constexpr size_t NAME_CHAR_LEN = 64;
inline constexpr size_t SYSTEM_CHARSET_MBMAXLEN = 3;
inline constexpr size_t NAME_LEN = NAME_CHAR_LEN * SYSTEM_CHARSET_MBMAXLEN;

inline constexpr uint32_t BINLOG_MAGIC = 0x6e6962fe;

inline constexpr int EVENT_TYPE_OFFSET = 4;

struct Uuid {
  static constexpr size_t BYTE_LENGTH = 16;

  std::array<unsigned char, BYTE_LENGTH> bytes;
};

struct Tag {
  using Tag_data = std::string;

  Tag_data m_data = "";
};
} // namespace mysql_binlog

#endif