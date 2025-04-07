#ifndef _BINLOG_READER_HPP
#define _BINLOG_READER_HPP

#include <istream>
#include <vector>

#include <binlog/binlog_events.hpp>

namespace mysql_binlog::reader {

int read(const char* file_path,
         std::vector<mysql_binlog::event::BinlogEvent::Ptr>& storage);

}; // namespace mysql_binlog::reader

#endif