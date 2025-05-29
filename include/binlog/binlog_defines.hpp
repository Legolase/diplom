#ifndef _BINLOG_DEFINES_HPP
#define _BINLOG_DEFINES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace binlog {
inline constexpr size_t BINLOG_VERSION = 4;
inline constexpr size_t ST_SERVER_VER_LEN = 50;
inline constexpr size_t ST_SERVER_VER_SPLIT_LEN = 3;
inline constexpr size_t LOG_EVENT_HEADER_LEN = 19;
inline constexpr size_t BINLOG_CHECKSUM_ALG_DESC_LEN = 1;
inline constexpr size_t MAX_DBS_IN_EVENT_MTS = 16;
inline constexpr size_t NAME_CHAR_LEN = 64;
inline constexpr size_t SYSTEM_CHARSET_MBMAXLEN = 3;
inline constexpr size_t CHECKSUM_CRC32_SIGNATURE_LEN = 4;
inline constexpr size_t NAME_LEN = NAME_CHAR_LEN * SYSTEM_CHARSET_MBMAXLEN;
inline constexpr size_t ROTATE_EVENT_MAX_FULL_NAME_SIZE = 512;
inline constexpr char SERVER_VERSION[] = "8.0.41";

inline constexpr uint32_t BINLOG_MAGIC = 0x6e6962fe;

inline constexpr int EVENT_TYPE_OFFSET = 4;
inline constexpr int DATA_WRITTEN_OFFSET = 9;

constexpr unsigned char checksum_version_split[3] = {5, 6, 1};
constexpr unsigned long checksum_version_product =
    (checksum_version_split[0] * 256 + checksum_version_split[1]) * 256 +
    checksum_version_split[2];

struct Uuid {
  static constexpr size_t BYTE_LENGTH = 16;

  std::array<unsigned char, BYTE_LENGTH> bytes;
};

struct Tag {
  using Tag_data = std::string;

  Tag_data m_data = "";
};
} // namespace binlog

#endif