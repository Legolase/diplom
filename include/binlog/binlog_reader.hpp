#ifndef _BINLOG_READER_HPP
#define _BINLOG_READER_HPP

#include <istream>
#include <vector>

#include <binlog/binlog_events.hpp>

namespace binlog::reader {

int read(const char* file_path, std::vector<binlog::event::BinlogEvent::UPtr>& storage);

}; // namespace binlog::reader

#endif