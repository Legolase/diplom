#ifndef _BINLOG_READER_HPP
#define _BINLOG_READER_HPP

#include <istream>
#include <vector>

#include <binlog/binlog_events.hpp>

namespace mysql_binlog::reader {

std::vector<mysql_binlog::event::BinlogEvent::Ptr> read() {}

}; // namespace mysql_binlog::reader

#endif