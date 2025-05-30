#include <binlog/binlog_events.hpp>
#include <cdc/cdc.hpp>

#include <components/document/document.hpp>
#include <concepts>
#include <utility>

#define READ(value, reader) ((value) = (reader).read<decltype(value)>())
#define PEEK(value, reader, ...) ((value) = (reader).peek<decltype(value)>(__VA_ARGS__))

namespace {

template<std::ranges::range T>
void toVectorBool(std::vector<bool>& result, const T& source)
{
  using TValueType = std::remove_cvref_t<decltype(*source.begin())>;
  static constexpr size_t value_type_size = sizeof(TValueType) * 8;

  result.clear();
  result.reserve(source.size() * value_type_size);
  [&]<size_t... Ind>(std::index_sequence<Ind...>) {
    for (const auto& elem : source) {
      (result.push_back((elem >> Ind) & 1), ...);
    }
  }(std::make_index_sequence<value_type_size>{});
}

void setValue(
    components::document::document_ptr doc, std::string_view json_pointer,
    utils::StringBufferReader& column_type_r,
    utils::StringBufferReader& column_metatype_r, utils::StringBufferReader& row_r,
    const cdc::TableDiff& data
)
{
  using namespace binlog::event;
  TableMapEvent::ColumnType type;
  int current_signedness_index = 0;

  READ(type, column_type_r);

  switch (type) {
  case TableMapEvent::TYPE_TINY:
  case TableMapEvent::TYPE_SHORT:
  case TableMapEvent::TYPE_INT24:
  case TableMapEvent::TYPE_LONG:
  case TableMapEvent::TYPE_LONGLONG:
  case TableMapEvent::TYPE_FLOAT:
  case TableMapEvent::TYPE_DOUBLE:
  case TableMapEvent::TYPE_BIT:
  case TableMapEvent::TYPE_BOOL:
  case TableMapEvent::TYPE_VARCHAR:
  case TableMapEvent::TYPE_STRING:
    break;
  default:
    THROW(std::runtime_error, "Unexpected type");
  }
}

} // namespace

namespace cdc {

DBBufferSource::DBBufferSource(
    const char* host, const char* user, const char* passwd, const char* db,
    unsigned int port
) :
    fde(binlog::BINLOG_VERSION, binlog::SERVER_VERSION)
{
  mysql_init(&conn);

  if (!mysql_real_connect(&conn, host, user, passwd, db, port, nullptr, 0)) {
    LOG_ERROR() << "     Error: " << mysql_error(&conn);
    LOG_ERROR() << "Error code: " << mysql_errno(&conn);
    THROW(std::runtime_error, fmt::format("Can't connect to `{}`", db));
  }

  rpl.file_name_length = 0;
  rpl.file_name = nullptr;
  rpl.start_position = 4;
  rpl.server_id = 0;
  rpl.flags = 0;

  if (mysql_binlog_open(&conn, &rpl)) {
    mysql_close(&conn);
    THROW(std::runtime_error, "Can't open binlog source");
  }
  LOG_INFO() << fmt::format("Connected to `{}`", db);
}

DBBufferSource::~DBBufferSource()
{
  std::string db = conn.db;

  mysql_binlog_close(&conn, &rpl);
  mysql_close(&conn);

  LOG_INFO() << fmt::format("Connection to {} successfully closed", db);
}

std::optional<Buffer> DBBufferSource::getDataImpl()
{
  while (true) {
    if (mysql_binlog_fetch(&conn, &rpl)) {
      LOG_ERROR() << fmt::format("Failed to read binlog from db '{}'", conn.db);
      LOG_ERROR(
      ) << fmt::format("Error code {}: {}", mysql_errno(&conn), mysql_error(&conn));
      THROW(std::runtime_error, "Binlog receive error");
    }

    if (rpl.size == 0) {
      LOG_WARNING() << fmt::format("EOF catched from db '{}'", conn.db);
      return {};
    }

    std::string_view str_view(reinterpret_cast<const char*>(rpl.buffer), rpl.size);

    // check is rotation required
    utils::StringBufferReader reader(str_view);
    binlog::event::LogEventType event_type;

    PEEK(event_type, reader);

    switch (event_type) {
    case binlog::event::ROTATE_EVENT:
      if (!rotate(reader)) {
        THROW(std::runtime_error, "Exception in rotation process of binlog file");
      }
      break;
    case binlog::event::FORMAT_DESCRIPTION_EVENT:
      formatDescription(reader);
    default:
      return str_view;
    }
  }
}

bool DBBufferSource::rotate(utils::StringBufferReader& reader)
{
  mysql_binlog_close(&conn, &rpl);
  binlog::event::RotateEvent rotate_event(reader, &fde);
  file_path_opt = rotate_event.new_log_ident;

  rpl.file_name = file_path_opt.value().c_str();
  rpl.file_name_length = file_path_opt.value().size();
  rpl.start_position = rotate_event.pos;

  return mysql_binlog_open(&conn, &rpl) == 0;
}

void DBBufferSource::formatDescription(utils::StringBufferReader& reader)
{
  fde = binlog::event::FormatDescriptionEvent(reader, &fde);
}

EventSource::EventSource(BufferSourceI::UPtr buffer_source, DataHandler data_handler) :
    EventSourceI(data_handler),
    buffer_source(std::move(buffer_source))
{}

std::optional<Binlog> EventSource::getDataImpl()
{
  const auto data = buffer_source->getData();
  if (!data) {
    return std::nullopt;
  }
  using namespace binlog;
  static event::FormatDescriptionEvent::SPtr fde =
      std::make_shared<event::FormatDescriptionEvent>(BINLOG_VERSION, SERVER_VERSION);

  utils::StringBufferReader reader(data->data(), data->size());
  event::LogEventType event_type;
  event::BinlogEvent::UPtr ev;
  PEEK(event_type, reader, EVENT_TYPE_OFFSET);

  switch (event_type) {
  case binlog::event::LogEventType::FORMAT_DESCRIPTION_EVENT:
    ev = std::make_unique<binlog::event::FormatDescriptionEvent>(reader, fde.get());
    break;
  case binlog::event::LogEventType::ROTATE_EVENT:
    ev = std::make_unique<binlog::event::RotateEvent>(reader, fde.get());
    break;
  case binlog::event::LogEventType::TABLE_MAP_EVENT:
    ev = std::make_unique<binlog::event::TableMapEvent>(reader, fde.get());
    break;
  case binlog::event::LogEventType::UPDATE_ROWS_EVENT_V1:
    ev = std::make_unique<binlog::event::UpdateRowsEvent>(reader, fde.get());
    break;
  case binlog::event::LogEventType::DELETE_ROWS_EVENT_V1:
    ev = std::make_unique<binlog::event::DeleteRowsEvent>(reader, fde.get());
    break;
  case binlog::event::LogEventType::WRITE_ROWS_EVENT_V1:
    ev = std::make_unique<binlog::event::WriteRowsEvent>(reader, fde.get());
    break;
  default:
    LOG_WARNING() << "Unknown event";
    goto return_ev;
  }

return_ev:
  return ev;
}

TableDiffSource::TableDiffSource(
    EventSourceI::UPtr event_source, DataHandler table_diff_handler
) :
    TableDiffSourceI(table_diff_handler),
    event_source(std::move(event_source))
{}

std::optional<TableDiff> TableDiffSource::getDataImpl()
{
  using namespace binlog;
  auto event_package = getEventPackage();

  if (!event_package.first || !event_package.second) {
    return std::nullopt;
  }

  auto& table_map_event = event_package.first;
  auto& rows_event = event_package.second;

  assert(table_map_event->m_table_id == rows_event->m_table_id);
  auto row_type = rows_event->m_type;
  TableDiff::Type type;

  switch (row_type) {
  case event::WRITE_ROWS_EVENT_V1:
    type = TableDiff::INSERT;
    break;
  case event::DELETE_ROWS_EVENT_V1:
    type = TableDiff::DELETE;
    break;
  case event::UPDATE_ROWS_EVENT_V1:
    type = TableDiff::UPDATE;
    break;
  }

  return TableDiff{
      .type = type,
      .collection_name = std::move(table_map_event->m_dbnam),
      .table_name = std::move(table_map_event->m_tblnam),
      .column_types = std::move(table_map_event->m_coltype),
      .column_metatypes = std::move(table_map_event->m_field_metadata),
      .column_name_list = table_map_event->getColumnName(),
      .column_primary_key_list = table_map_event->getSimplePrimaryKey(),
      .column_signedness = table_map_event->getSignedness(),
      .row = std::move(rows_event->row),
      .width = table_map_event->column_count
  };
}

TableDiffSource::EventPackage TableDiffSource::getEventPackage()
{
  using namespace binlog;
  event::TableMapEvent::UPtr table_map_event;
  event::RowsEvent::UPtr rows_event;
  // Loop until we got a pair of TableMapEvent and struct derived from RowsEvent
  while (true) {
    auto data = event_source->getData();
    if (!data) {
      return {nullptr, nullptr};
    }

    auto& data_binlog_uptr = data.value();
    switch (data_binlog_uptr->header.type_code) {
    case event::LogEventType::TABLE_MAP_EVENT: {
      if (table_map_event) {
        THROW(
            std::runtime_error,
            "Invalid event sequence -- expected RowsEvent, got TableMapEvent"
        );
      }
      table_map_event = std::unique_ptr<event::TableMapEvent>(
          static_cast<event::TableMapEvent*>(data_binlog_uptr.release())
      );
      break;
    }
    case event::LogEventType::WRITE_ROWS_EVENT_V1:
    case event::LogEventType::UPDATE_ROWS_EVENT_V1:
    case event::LogEventType::DELETE_ROWS_EVENT_V1: {
      if (!table_map_event) {
        THROW(
            std::runtime_error, "Invalid event sequence: Expected TableMapEvent before "
                                "RowsEvent, got RowsEvent"
        );
      }
#ifndef NDEBUG
      if (rows_event) {
        THROW(
            std::logic_error,
            "Invalid stage error: RowsEvent already got but asked for again"
        );
      }
#endif
      break;
    }
    }

    if (table_map_event && rows_event) {
      // Got all info. Can convert to TableDiff
      break;
    }
  }
  return {std::move(table_map_event), std::move(rows_event)};
}

OtterBrixDiffSink::OtterBrixDiffSink(
    OtterBrixConsumerI::UPtr otterbrix_consumer, std::pmr::memory_resource* resource
) :
    otterbrix_consumer(std::move(otterbrix_consumer)),
    resource(resource)
{}

void OtterBrixDiffSink::putDataImpl(const TableDiff& data)
{
  node_ptr result;

  switch (data.type) {
  case TableDiff::INSERT:
    sendNodesInsert(data);
    break;
  case TableDiff::DELETE:
    sendNodesDelete(data);
    break;
  case TableDiff::UPDATE:
    // sendNodesUpdate(data);
    break;
  }
}

OtterBrixDiffSink::ReadContext::ReadContext(const TableDiff& data) :
    column_metatype_r(
        reinterpret_cast<const char*>(data.column_metatypes.data()),
        data.column_metatypes.size()
    ),
    column_type_r(
        reinterpret_cast<const char*>(data.column_types.data()), data.column_types.size()
    ),
    row_r(reinterpret_cast<const char*>(data.row.data()), data.row.size()),
    signedness_r(data.column_signedness),
    data(data)
{}

void OtterBrixDiffSink::sendNodesInsert(const TableDiff& data)
{
  using namespace components::logical_plan;

  std::pmr::vector<components::document::document_ptr> docs;
  collection_full_name_t collection(data.collection_name, data.table_name);

  ReadContext context(data);

  docs.reserve(16);

  while (context.row_r.available()) {
    auto& doc = docs.emplace_back() = components::document::make_document(resource);
    fillDocument(doc, context);
  }

  otterbrix_consumer->putData(ExtendedNode{
      .node_ptr = make_node_insert(resource, collection, std::move(docs)),
      .parameter = nullptr
  });
}

void OtterBrixDiffSink::sendNodesDelete(const TableDiff& data)
{
  using namespace components::logical_plan;
  using namespace components::expressions;
  using namespace components::document;
  using param_t = core::parameter_id_t;

  collection_full_name_t collection(data.collection_name, data.table_name);

  ReadContext context(data);

  while (context.row_r.available()) {
    auto doc = components::document::make_document(resource);

    fillDocument(doc, context);

    auto selection_params = getSelectionParameters(doc, context);

    auto& expr = selection_params.first;
    auto& params = selection_params.second;

    otterbrix_consumer->putData(ExtendedNode{
        .node_ptr = make_node_delete_one(
            resource, collection, make_node_match(resource, collection, std::move(expr))
        ),
        .parameter = std::move(params)
    });
  }
}

void OtterBrixDiffSink::sendNodesUpdate(const TableDiff& data)
{
  using namespace components::logical_plan;
  collection_full_name_t collection(data.collection_name, data.table_name);

  ReadContext context(data);

  while (context.row_r.available()) {
    auto old_doc = components::document::make_document(resource);
    auto new_doc = components::document::make_document(resource);

    fillDocument(old_doc, context);
    fillDocument(new_doc, context);

    auto selection_params = getSelectionParameters(old_doc, context);
    auto& expr = selection_params.first;
    auto& params = selection_params.second;

    otterbrix_consumer->putData(ExtendedNode{
        .node_ptr = make_node_update_one(
            resource, collection, make_node_match(resource, collection, std::move(expr)),
            std::move(new_doc)
        ),
        .parameter = std::move(params)
    });
  }
}

void OtterBrixDiffSink::fillDocument(
    components::document::document_ptr& doc, ReadContext& context
)
{
  auto& json_pointer = context.cached_data.json_pointer;
  auto& null_bitmap = context.cached_data.null_bitmap;
  int null_bitmap_byte_size = (context.data.width + 7) / 8;

  toVectorBool(null_bitmap, std::string_view(context.row_r.ptr(), null_bitmap_byte_size));
  context.row_r.skip(null_bitmap_byte_size);

  for (int i = 0; i < context.data.width; ++i) {
    json_pointer = "/";
    json_pointer += context.data.column_name_list[i];
    if (null_bitmap[i]) {
      doc->set(json_pointer, nullptr);
    } else {
      using namespace binlog::event;
      TableMapEvent::ColumnType type;

      READ(type, context.column_type_r);

      switch (type) {
      case TableMapEvent::TYPE_TINY: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint8_t value = context.row_r.read<uint8_t>();
          doc->set(json_pointer, value);
        } else {
          int8_t value = context.row_r.read<int8_t>();
          doc->set(json_pointer, value);
        }
      }
      case TableMapEvent::TYPE_SHORT: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint16_t value = context.row_r.read<uint8_t>();
          doc->set(json_pointer, value);
        } else {
          int16_t value = context.row_r.read<int16_t>();
          doc->set(json_pointer, value);
        }
      }
      case TableMapEvent::TYPE_INT24: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint32_t value = 0;
          context.row_r.readCpy((char*)&value, 3);

          doc->set(json_pointer, value);
        } else {
          int32_t value;

          if (context.row_r.peek<uint8_t>(2) & 128) {
            value = (int32_t)-1 << 24;
          } else {
            value = 0;
          }

          context.row_r.readCpy((char*)&value, 3);
          doc->set(json_pointer, value);
        }
      }
      case TableMapEvent::TYPE_LONG: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint32_t value = context.row_r.read<uint32_t>();
          doc->set(json_pointer, value);
        } else {
          int32_t value = context.row_r.read<int32_t>();
          doc->set(json_pointer, value);
        }
      }
      case TableMapEvent::TYPE_LONGLONG: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint64_t value = context.row_r.read<uint64_t>();
          doc->set(json_pointer, value);
        } else {
          int64_t value = context.row_r.read<int64_t>();
          doc->set(json_pointer, value);
        }
      }
      case TableMapEvent::TYPE_FLOAT: {
        context.signedness_r.read();
        float value = context.row_r.read<float>();
        doc->set(json_pointer, value);
      }
      case TableMapEvent::TYPE_DOUBLE: {
        context.signedness_r.read();
        double value = context.row_r.read<double>();
        doc->set(json_pointer, value);
      }
      case TableMapEvent::TYPE_BOOL: {
        context.signedness_r.read();

        bool value = context.row_r.read<uint8_t>();
        doc->set(json_pointer, value);
      }
      case TableMapEvent::TYPE_VARCHAR: {
        uint16_t max_length = context.column_metatype_r.read<uint16_t>();

        uint16_t len = 0;
        if (max_length <= 255) {
          len = context.row_r.read<uint8_t>();
        } else {
          len = context.row_r.read<uint16_t>();
        }
        std::string str(0, len);
        context.row_r.readCpy(str.data(), str.size());

        doc->set(json_pointer, std::move(str));
      }
      case TableMapEvent::TYPE_STRING: {
        uint8_t real_type = context.column_metatype_r.read<uint8_t>();

        if (real_type == TableMapEvent::TYPE_STRING) {
          goto unexpected_type;
        }
        uint8_t& len = real_type;
        len = context.column_metatype_r.read<uint8_t>();

        std::string str(0, len);
        context.row_r.readCpy(str.data(), str.size());
        doc->set(json_pointer, std::move(str));
      } break;
      default: {
      unexpected_type:
        THROW(std::runtime_error, "Unexpected type");
      }
      }
    }
  }
}

std::pair<compare_expression_ptr, parameter_node_ptr>
OtterBrixDiffSink::getSelectionParameters(
    const components::document::document_ptr& doc, ReadContext& context
)
{
  using namespace components::expressions;
  using param_t = core::parameter_id_t;

  auto expr = components::expressions::make_compare_union_expression(
      resource, compare_type::union_and
  );
  auto params = components::logical_plan::make_parameter_node(resource);
  uint16_t current_parameter_id = 1;

  for (int i = 0; i < context.data.column_primary_key_list.size();
       ++i, ++current_parameter_id)
  {
    const auto primary_key_index = context.data.column_primary_key_list[i];
    const auto& field_name = context.data.column_name_list[primary_key_index];
    const auto& json_pointer = (context.cached_data.json_pointer = "/") += field_name;
    const auto logic_type = doc->type_by_key(json_pointer);

    auto add_parameter = [this, &expr, &field_name, &current_parameter_id,
                          &params](const auto& value) {
      expr->append_child(components::expressions::make_compare_expression(
          resource, compare_type::eq, components::expressions::key_t{field_name},
          core::parameter_id_t{current_parameter_id}
      ));
      params->add_parameter(param_t{current_parameter_id}, value);
    };

    switch (logic_type) {
    case components::types::logical_type::NA: {
      THROW(
          std::runtime_error,
          "Assertion error. At least one column of primary key has null value"
      );
    }
    case components::types::logical_type::TINYINT: {
      add_parameter(doc->get_tinyint(json_pointer));
      break;
    }
    case components::types::logical_type::SMALLINT: {
      add_parameter(doc->get_smallint(json_pointer));
      break;
    }
    case components::types::logical_type::INTEGER: {
      add_parameter(doc->get_int(json_pointer));
      break;
    }
    case components::types::logical_type::BIGINT: {
      add_parameter(doc->get_long(json_pointer));
      break;
    }
    case components::types::logical_type::UTINYINT: {
      add_parameter(doc->get_utinyint(json_pointer));
      break;
    }
    case components::types::logical_type::USMALLINT: {
      add_parameter(doc->get_usmallint(json_pointer));
      break;
    }
    case components::types::logical_type::UINTEGER: {
      add_parameter(doc->get_uint(json_pointer));
      break;
    }
    case components::types::logical_type::UBIGINT: {
      add_parameter(doc->get_ulong(json_pointer));
      break;
    }
    case components::types::logical_type::FLOAT: {
      add_parameter(doc->get_float(json_pointer));
      break;
    }
    case components::types::logical_type::DOUBLE: {
      add_parameter(doc->get_double(json_pointer));
      break;
    }
    case components::types::logical_type::BOOLEAN: {
      add_parameter(doc->get_bool(json_pointer));
      break;
    }
    case components::types::logical_type::STRING_LITERAL: {
      add_parameter(doc->get_string(json_pointer));
      break;
    }
    default:
      THROW(std::runtime_error, "Expected known type");
    }
  }

  return {std::move(expr), std::move(params)};
}

OtterBrixConsumerSink::OtterBrixConsumerSink(DataHandler data_handler) :
    OtterBrixConsumerI(data_handler)
{}

void OtterBrixConsumerSink::putDataImpl(const ExtendedNode& extended_node) {}
} // namespace cdc