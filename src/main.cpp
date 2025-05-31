#include <binlog/binlog_reader.hpp>
#include <cdc/cdc.hpp>
#include <iostream>
#include <sstream>

template<typename T, typename... Args>
decltype(auto) uptr(Args&&... args)
{
  return std::make_unique<T>(std::forward<Args>(args)...);
}

int main(int argc, char** argv)
{
  auto db_reader_source =
      uptr<cdc::DBBufferSource>("127.0.0.1", "root", "person", "e_store", 3306);
  auto event_source =
      uptr<cdc::EventSource>(std::move(db_reader_source), [](const cdc::Binlog& binlog) {
      });

  auto table_diff_source = uptr<cdc::TableDiffSource>(
      std::move(event_source),
      [](const cdc::TableDiff& table_diff) {
      }
  );

  std::pmr::synchronized_pool_resource resource;
  auto otterbrix_consumer =
      uptr<cdc::OtterBrixConsumerSink>([](const cdc::ExtendedNode&) {
      });
  auto otterbrik_diff_sink =
      uptr<cdc::OtterBrixDiffSink>(std::move(otterbrix_consumer), &resource);
  auto main_process = uptr<cdc::MainProcess>(
      std::move(table_diff_source), std::move(otterbrik_diff_sink)
  );

  main_process->process();
  // const char* file_path = "./static/binlog/mysql-bin.000002";
  // std::vector<binlog::event::BinlogEvent::UPtr> v;

  // binlog::reader::read(file_path, v);
}
