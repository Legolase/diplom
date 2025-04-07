#include <iostream>

#include <binlog/binlog_reader.hpp>

int main(int argc, char** argv) {
  std::vector<mysql_binlog::event::BinlogEvent::Ptr> events;

  return mysql_binlog::reader::read("./binlog/mysql-bin.000003", events);
}
