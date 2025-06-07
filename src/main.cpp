#include <binlog/binlog_reader.hpp>
#include <cdc/cdc.hpp>
#include <iostream>
#include <sstream>

#include <components/document/value.hpp>

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
      uptr<cdc::EventSource>(std::move(db_reader_source), [](const cdc::Binlog& ev) {});

  auto table_diff_source = uptr<cdc::TableDiffSource>(
      std::move(event_source),
      [](const cdc::TableDiff& table_diff) {
      }
  );

  auto otterbrix_consumer =
      uptr<cdc::OtterBrixConsumerSink>([](const cdc::ExtendedNode& e_node) {});
  auto otterbrik_diff_sink = uptr<cdc::OtterBrixDiffSink>(
      std::move(otterbrix_consumer), otterbrix_consumer->resource()
  );
  auto main_process = uptr<cdc::MainProcess>(
      std::move(table_diff_source), std::move(otterbrik_diff_sink)
  );

  main_process->process();
}
