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
template <typename ProcessFunc>
binlog::event::BinlogEvent::UPtr
processEvents(utils::StreamReader& reader, ProcessFunc&& func)
{
  using namespace binlog;

  event::FormatDescriptionEvent::SPtr fde =
      std::make_shared<event::FormatDescriptionEvent>(BINLOG_VERSION, SERVER_VERSION);
  std::string buffer;
  uint32_t event_size;
  event::LogEventType event_type;
  buffer.reserve(1024);

  while (true) {
    bool process_event = true;
    PEEK(reader, event_size, DATA_WRITTEN_OFFSET);
    PEEK(reader, event_type, EVENT_TYPE_OFFSET);
    buffer.resize(event_size);

    reader.readCpy(buffer.data(), buffer.size());

    utils::StringBufferReader event_reader(buffer.data(), event_size);

    binlog::event::BinlogEvent::SPtr ev;

    switch (event_type) {
    case binlog::event::LogEventType::FORMAT_DESCRIPTION_EVENT:
      ev = std::make_shared<binlog::event::FormatDescriptionEvent>(
          event_reader, fde.get()
      );
      break;
    case binlog::event::LogEventType::ROTATE_EVENT:
      ev = std::make_shared<binlog::event::RotateEvent>(event_reader, fde.get());
      break;
    case binlog::event::LogEventType::TABLE_MAP_EVENT:
      ev = std::make_shared<binlog::event::TableMapEvent>(event_reader, fde.get());
      break;
    case binlog::event::LogEventType::UPDATE_ROWS_EVENT_V1:
      ev = std::make_shared<binlog::event::UpdateRowsEvent>(event_reader, fde.get());
      break;
    case binlog::event::LogEventType::DELETE_ROWS_EVENT_V1:
      ev = std::make_shared<binlog::event::DeleteRowsEvent>(event_reader, fde.get());
      break;
    case binlog::event::LogEventType::WRITE_ROWS_EVENT_V1:
      ev = std::make_shared<binlog::event::WriteRowsEvent>(event_reader, fde.get());
      break;
    default:
      LOG_WARNING() << "Unknown event";
      process_event = false;
    }

    if (!process_event) {
      continue;
    }
    func(ev);
    if (ev->header.type_code == binlog::event::LogEventType::FORMAT_DESCRIPTION_EVENT) {
      fde = std::static_pointer_cast<binlog::event::FormatDescriptionEvent>(ev);
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

  // TODO: Not sure if it's good idea to stop reading stream after catching exception
  try {
    const auto ev = processEvents(reader, [](const BinlogEvent::SPtr& ev) {
      switch (ev->header.type_code) {
      case LogEventType::WRITE_ROWS_EVENT_V1: {
        WriteRowsEvent::SPtr row_event = std::static_pointer_cast<WriteRowsEvent>(ev);
        row_event->show();
        break;
      }
      case LogEventType::UPDATE_ROWS_EVENT_V1: {
        UpdateRowsEvent::SPtr row_event = std::static_pointer_cast<UpdateRowsEvent>(ev);
        row_event->show();
        break;
      }
      case LogEventType::DELETE_ROWS_EVENT_V1: {
        DeleteRowsEvent::SPtr row_event = std::static_pointer_cast<DeleteRowsEvent>(ev);
        row_event->show();
        break;
      }
      case LogEventType::FORMAT_DESCRIPTION_EVENT: {
        FormatDescriptionEvent::SPtr fde_event =
            std::static_pointer_cast<FormatDescriptionEvent>(ev);
        fde_event->show();
        break;
      }
      case LogEventType::ROTATE_EVENT: {
        RotateEvent::SPtr rotate_event = std::static_pointer_cast<RotateEvent>(ev);
        rotate_event->show();
        break;
      }
      case LogEventType::TABLE_MAP_EVENT: {
        TableMapEvent::SPtr table_event = std::static_pointer_cast<TableMapEvent>(ev);
        table_event->TableMapEvent::show();
        break;
      }
      default:
        LOG_INFO() << "Unknown event";
      }
      LOG_INFO() << "=======================================";
    });

  } catch (utils::BadStream& e) {
    LOG_WARNING() << e.what();
    LOG_INFO() << "End of reading stream";
  }

  return 0;
}

} // namespace binlog::reader
