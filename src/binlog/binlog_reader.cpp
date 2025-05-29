#include <binlog/binlog_defines.hpp>
#include <binlog/binlog_events.hpp>
#include <binlog/binlog_reader.hpp>
#include <utils/stream_reader.hpp>
#include <utils/string_buffer_reader.hpp>

#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <type_traits>

#define READ(reader, value) ((value) = reader.read<decltype(value)>())
#define PEEK(reader, value, ...) ((value) = reader.peek<decltype(value)>(__VA_ARGS__))

namespace {
template<typename ProcessFunc>
void processEvents(utils::StreamReader& reader, ProcessFunc&& func)
{
  using namespace binlog;
  using namespace binlog::event;

  FormatDescriptionEvent::UPtr fde =
      std::make_unique<FormatDescriptionEvent>(BINLOG_VERSION, SERVER_VERSION);
  std::string buffer;
  uint32_t event_size;
  LogEventType event_type;
  buffer.reserve(1024);

  while (!reader.isEnd()) {
    bool process_event = true;
    PEEK(reader, event_size, DATA_WRITTEN_OFFSET);
    PEEK(reader, event_type, EVENT_TYPE_OFFSET);
    buffer.resize(event_size);

    reader.readCpy(buffer.data(), buffer.size());

    utils::StringBufferReader event_reader(buffer.data(), event_size);

    BinlogEvent::UPtr ev;

    switch (event_type) {
    case LogEventType::FORMAT_DESCRIPTION_EVENT:
      ev = std::make_unique<FormatDescriptionEvent>(event_reader, fde.get());
      break;
    case LogEventType::ROTATE_EVENT:
      ev = std::make_unique<RotateEvent>(event_reader, fde.get());
      break;
    case LogEventType::TABLE_MAP_EVENT:
      ev = std::make_unique<TableMapEvent>(event_reader, fde.get());
      break;
    case LogEventType::UPDATE_ROWS_EVENT_V1:
      ev = std::make_unique<UpdateRowsEvent>(event_reader, fde.get());
      break;
    case LogEventType::DELETE_ROWS_EVENT_V1:
      ev = std::make_unique<DeleteRowsEvent>(event_reader, fde.get());
      break;
    case LogEventType::WRITE_ROWS_EVENT_V1:
      ev = std::make_unique<WriteRowsEvent>(event_reader, fde.get());
      break;
    default:
      LOG_WARNING() << "Unknown event";
      process_event = false;
    }

    if (!process_event) {
      continue;
    }
    func(ev);
    if (ev->header.type_code == LogEventType::FORMAT_DESCRIPTION_EVENT) {
      fde.reset(static_cast<FormatDescriptionEvent*>(ev.release()));
    }
  }
}
} // namespace

namespace binlog::reader {

int read(const char* file_path, std::vector<binlog::event::BinlogEvent::UPtr>& storage)
{
  using namespace binlog::event;
  std::ifstream binlog_file(file_path, std::ios::in | std::ios::binary);

  uint32_t magic_num;

  if (!binlog_file) {
    LOG_ERROR() << fmt::format("binlog file '{}' not found", file_path);
    return -1;
  }

  utils::StreamReader reader(binlog_file);
  try {
    READ(reader, magic_num);

    if (magic_num != BINLOG_MAGIC) {
      LOG_ERROR() << fmt::format("File {} is not a mysql binlog file", file_path);
      return -1;
    }
  } catch (utils::BadStream& e) {
    LOG_ERROR() << "utils::BadStream error: " << e.what();
    return -1;
  }

  processEvents(reader, [](const BinlogEvent::UPtr& ev) {
    switch (ev->header.type_code) {
    case LogEventType::WRITE_ROWS_EVENT_V1: {
      const auto* row_event = static_cast<const WriteRowsEvent*>(ev.get());
      row_event->show();
      break;
    }
    case LogEventType::UPDATE_ROWS_EVENT_V1: {
      const auto* row_event = static_cast<const UpdateRowsEvent*>(ev.get());
      row_event->show();
      break;
    }
    case LogEventType::DELETE_ROWS_EVENT_V1: {
      const auto* row_event = static_cast<const DeleteRowsEvent*>(ev.get());
      row_event->show();
      break;
    }
    case LogEventType::FORMAT_DESCRIPTION_EVENT: {
      const auto* fde_event = static_cast<const FormatDescriptionEvent*>(ev.get());
      fde_event->show();
      break;
    }
    case LogEventType::ROTATE_EVENT: {
      const auto* rotate_event = static_cast<const RotateEvent*>(ev.get());
      rotate_event->show();
      break;
    }
    case LogEventType::TABLE_MAP_EVENT: {
      const auto* table_event = static_cast<const TableMapEvent*>(ev.get());
      table_event->TableMapEvent::show();
      break;
    }
    default:
      LOG_INFO() << "Unknown event";
    }
    LOG_INFO() << "=======================================";
  });

  return 0;
}

} // namespace binlog::reader
