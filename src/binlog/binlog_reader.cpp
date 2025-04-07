#include <binlog/binlog_defines.hpp>
#include <binlog/binlog_reader.hpp>

#include <fmt/format.h>
#include <fstream>
#include <iostream>

namespace {
template <typename T>
int readb(T& value, std::istream& in, int offset = 0) {
  if (offset) {
    const auto cur_pos = in.tellg();
    in.seekg(offset, std::ios::cur);
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    in.seekg(cur_pos);
  } else {
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
  }

  if (!in) {
    return -1;
  }
  return 0;
}
} // namespace

namespace mysql_binlog::reader {

int read(const char* file_path,
         std::vector<mysql_binlog::event::BinlogEvent::Ptr>& storage) {

  std::ifstream binlog_file(file_path, std::ios::in | std::ios::binary);
  uint32_t magic_num;
  uint8_t event_type_code;

  if (!binlog_file) {
    std::cerr << fmt::format("[Error] binlog file '{}' not found", file_path);
    return -1;
  }

  if (readb(magic_num, binlog_file) != 0 || magic_num != BINLOG_MAGIC) {
    std::cerr << fmt::format("[Error] file '{}' is not binlog file", file_path);
    return -1;
  }

  readb(event_type_code, binlog_file, EVENT_TYPE_OFFSET);



  return 0;
}

} // namespace mysql_binlog::reader
