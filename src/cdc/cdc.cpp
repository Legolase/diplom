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

std::pmr::string gen_id(uint64_t num, std::pmr::memory_resource* resource)
{
  static constexpr int id_len = 24;
  std::pmr::string str_num{std::to_string(num), resource};
  std::pmr::string result(resource);
  result.reserve(id_len);
  result.resize(id_len - str_num.size(), '0');
  return result + str_num;
}

} // namespace

namespace cdc {

DBBufferSource::DBBufferSource(
    const char* host, const char* user, const char* passwd, const char* db,
    unsigned int port
) :
    host(host),
    user(user),
    passwd(passwd),
    db(db),
    port(port)
{
  mysql_init(&conn);
  connect();
}

DBBufferSource::~DBBufferSource()
{
  disconnect();
}

std::optional<Buffer> DBBufferSource::getDataImpl()
{
  while (true) {
    const auto& buffer = nextEventBuffer();

    process(buffer);

    return buffer;
  }
}

void DBBufferSource::connect()
{
  if (!mysql_real_connect(
          &conn, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0
      ))
  {
    LOG_ERROR() << "     Error: " << mysql_error(&conn);
    LOG_ERROR() << "Error code: " << mysql_errno(&conn);
    THROW(DBConnectionError, fmt::format("Can't connect to `{}`", db));
  }

  // To update position of next not processed event
  rotate();

  if (mysql_binlog_open(&conn, &rpl)) {
    mysql_close(&conn);
    THROW(DBBinlogError, "Can't open binlog source");
  }
  LOG_INFO() << fmt::format("Connected to `{}`", db);
}

void DBBufferSource::disconnect()
{
  mysql_binlog_close(&conn, &rpl);
  mysql_close(&conn);

  LOG_INFO() << fmt::format("Connection to {} successfully closed", db);
}

void DBBufferSource::rotate()
{
  rpl.file_name = file_path.c_str();
  rpl.file_name_length = file_path.size();
  rpl.start_position = next_pos;
  rpl.server_id = 0;
  rpl.flags = 0;
  fde = {binlog::BINLOG_VERSION, binlog::SERVER_VERSION};

  LOG_DEBUG() << "Rotation:";
  LOG_DEBUG() << "         rpl.file_name: " << rpl.file_name;
  LOG_DEBUG() << "  rpl.file_name_length: " << rpl.file_name_length;
  LOG_DEBUG() << "    rpl.start_position: " << rpl.start_position;
}

std::string_view DBBufferSource::nextEventBuffer()
{
  while (true) {
    if (mysql_binlog_fetch(&conn, &rpl)) {
      LOG_ERROR() << fmt::format("Failed to read binlog from db '{}'", conn.db);
      LOG_ERROR(
      ) << fmt::format("Error code {}: {}", mysql_errno(&conn), mysql_error(&conn));
      THROW(DBBinlogError, "Binlog receive event error");
    }

    if (rpl.size != 0) {
      utils::StringBufferReader reader(
          reinterpret_cast<const char*>(rpl.buffer + 1), rpl.size - 1
      );
      decltype(next_pos) new_next_pos;

      PEEK(new_next_pos, reader, binlog::LOG_POS_OFFSET);

      if (new_next_pos) {
        next_pos = new_next_pos;
      } else {
        LOG_DEBUG() << R"_(Received "virtual" event without valid log_pos value.)_";
      }

      return std::string_view(
          reinterpret_cast<const char*>(rpl.buffer + 1), rpl.size - 1
      );
    }
    LOG_DEBUG() << "Received binary event buffer is empty.";
    LOG_DEBUG() << fmt::format("Attempt to reconnect to '{}' database.", db);
    disconnect();
    connect();
  }
}

void DBBufferSource::process(std::string_view buffer)
{
  utils::StringBufferReader reader(buffer);

  binlog::event::LogEventType event_type;
  uint32_t event_size;

  PEEK(event_type, reader, binlog::EVENT_TYPE_OFFSET);
  PEEK(event_size, reader, binlog::DATA_WRITTEN_OFFSET);

  switch (event_type) {
  case binlog::event::ROTATE_EVENT: {
    fde = {binlog::BINLOG_VERSION, binlog::SERVER_VERSION};
    binlog::event::RotateEvent rotate_event(reader, &fde);
    file_path = std::move(rotate_event.new_log_ident);
    next_pos = rotate_event.pos;
    break;
  }
  case binlog::event::FORMAT_DESCRIPTION_EVENT: {
    fde = binlog::event::FormatDescriptionEvent(reader, &fde);
    break;
  }
  }
}

EventSource::EventSource(BufferSourceI::UPtr buffer_source, DataHandler data_handler) :
    EventSourceI(data_handler),
    buffer_source(std::move(buffer_source))
{}

std::optional<Binlog> EventSource::getDataImpl()
{
  using namespace binlog;
  event::BinlogEvent::UPtr ev;

  while (!ev) {
    const auto data = buffer_source->getData();
    if (!data) {
      return std::nullopt;
    }
    static event::FormatDescriptionEvent::SPtr fde =
        std::make_shared<event::FormatDescriptionEvent>(BINLOG_VERSION, SERVER_VERSION);

    utils::StringBufferReader reader(data->data(), data->size());
    event::LogEventType event_type;
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
      LOG_DEBUG() << "Unknown event";
    }
  }

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
      table_map_event = std::unique_ptr<event::TableMapEvent>(
          static_cast<event::TableMapEvent*>(data_binlog_uptr.release())
      );

      submitTableInfo(std::move(table_map_event));

      break;
    }
    case event::LogEventType::WRITE_ROWS_EVENT_V1:
    case event::LogEventType::UPDATE_ROWS_EVENT_V1:
    case event::LogEventType::DELETE_ROWS_EVENT_V1: {
      switch (data_binlog_uptr->header.type_code) {
      case event::LogEventType::WRITE_ROWS_EVENT_V1:
        rows_event = std::unique_ptr<event::WriteRowsEvent>(
            static_cast<event::WriteRowsEvent*>(data_binlog_uptr.release())
        );
        break;
      case event::LogEventType::UPDATE_ROWS_EVENT_V1:
        rows_event = std::unique_ptr<event::UpdateRowsEvent>(
            static_cast<event::UpdateRowsEvent*>(data_binlog_uptr.release())
        );
        break;
      case event::LogEventType::DELETE_ROWS_EVENT_V1:
        rows_event = std::unique_ptr<event::DeleteRowsEvent>(
            static_cast<event::DeleteRowsEvent*>(data_binlog_uptr.release())
        );
        break;
      }
      break;
    }
    }

    if (rows_event) {
      if (!(table_map_event = extractTableInfo(rows_event->m_table_id))) {
        THROW(
            TableDiffSourceError, fmt::format(
                                      "Expected existance info for table with id({}) "
                                      "before submiting rows event.",
                                      rows_event->m_table_id
                                  )
        );
      }
      // Got all info. Can convert to TableDiff
      break;
    }
  }
  return {std::move(table_map_event), std::move(rows_event)};
}

void TableDiffSource::submitTableInfo(TablePtr&& tm_event)
{
  if (!tm_event) {
    return;
  }

  const auto& table_id = tm_event->m_table_id;

  auto it = table_info_map.find(table_id);

  if (it == table_info_map.end()) {
    table_info_map.emplace(table_id, std::move(tm_event));
    return;
  }

  it->second = std::move(tm_event);
}

TableDiffSource::TablePtr TableDiffSource::extractTableInfo(const uint64_t table_id)
{
  auto it = table_info_map.find(table_id);

  if (it == table_info_map.end()) {
    return nullptr;
  }

  return TablePtr(it->second.release());
}

const std::string OtterBrixDiffSink::PK_FIELD_NAME = "_id";
const std::string OtterBrixDiffSink::PK_JSON_POINTER = std::string{"/"} + PK_FIELD_NAME;

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
    sendNodesUpdate(data);
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
    docs.emplace_back() = getDocument(context);
  }

  otterbrix_consumer->putData(ExtendedNode{
      .node = make_node_insert(resource, collection, std::move(docs)),
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
    auto doc = getDocument(context);

    auto selection_params = getSelectionParameters(doc, context);

    auto& expr = selection_params.first;
    auto& params = selection_params.second;

    otterbrix_consumer->putData(ExtendedNode{
        .node = make_node_delete_one(
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
    auto old_doc = getDocument(context);
    auto new_doc = getDocument(context);
    auto set_doc = components::document::make_document(resource);

    auto selection_params = getSelectionParameters(old_doc, context);
    auto& expr = selection_params.first;
    auto& params = selection_params.second;

    new_doc->remove(PK_JSON_POINTER);
    set_doc->set("$set", new_doc);

    otterbrix_consumer->putData(ExtendedNode{
        .node = make_node_update_one(
            resource, collection, make_node_match(resource, collection, std::move(expr)),
            std::move(set_doc)
        ),
        .parameter = std::move(params)
    });
  }
}

components::document::document_ptr OtterBrixDiffSink::getDocument(ReadContext& context)
{
  auto doc = components::document::make_document(resource);
  const auto pk_index = getPrimaryKeyIndex(context);

  if (pk_index == -1) {
    THROW(
        OtterBrixDiffSinkError,
        fmt::format(
            "Table {}.{}. Primary key must be one and name == '{}'.",
            context.data.collection_name, context.data.table_name, PK_FIELD_NAME
        )
    );
  }

  auto& json_pointer = context.cached_data.json_pointer;
  auto& null_bitmap = context.cached_data.null_bitmap;
  int null_bitmap_byte_size = (context.data.width + 7) / 8;

  context.column_type_r.restart();
  context.column_metatype_r.restart();
  context.signedness_r.restart();

  toVectorBool(null_bitmap, std::string_view(context.row_r.ptr(), null_bitmap_byte_size));
  context.row_r.skip(null_bitmap_byte_size);

  for (int i = 0; i < context.data.width; ++i) {
    json_pointer = "/";
    json_pointer += context.data.column_name_list[i];
    if (i == pk_index) {
      if (null_bitmap[i]) {
        THROW(
            OtterBrixDiffSinkError,
            fmt::format(
                "Table: {}.{}. Primary key '{}' must have value.",
                context.data.table_name, context.data.collection_name,
                context.data.column_name_list[pk_index]
            )
        );
      }
      using namespace binlog::event;
      TableMapEvent::ColumnType type;
      auto unsigned_ = context.signedness_r.read();
      READ(type, context.column_type_r);

      if (type != TableMapEvent::TYPE_LONGLONG || !unsigned_) {
        THROW(
            OtterBrixDiffSinkError,
            fmt::format(
                "Table: {}.{}. Type of primary key '{}' must have unsigned integral.",
                context.data.table_name, context.data.collection_name,
                context.data.column_name_list[pk_index]
            )
        );
      }
      uint64_t value = context.row_r.read<uint64_t>();
      doc->set(json_pointer, gen_id(value, resource));
    } else if (null_bitmap[i]) {
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
          doc->set<uint64_t>(json_pointer, value);
        } else {
          int8_t value = context.row_r.read<int8_t>();
          doc->set<int64_t>(json_pointer, value);
        }
        break;
      }
      case TableMapEvent::TYPE_SHORT: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint16_t value = context.row_r.read<uint16_t>();
          doc->set<uint64_t>(json_pointer, value);
        } else {
          int16_t value = context.row_r.read<int16_t>();
          doc->set<int64_t>(json_pointer, value);
        }
        break;
      }
      case TableMapEvent::TYPE_INT24: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint32_t value = 0;
          context.row_r.readCpy((char*)&value, 3);

          doc->set<uint64_t>(json_pointer, value);
        } else {
          int32_t value;

          if (context.row_r.peek<uint8_t>(2) & 128) {
            value = (int32_t)-1 << 24;
          } else {
            value = 0;
          }

          context.row_r.readCpy((char*)&value, 3);
          doc->set<int64_t>(json_pointer, value);
        }
        break;
      }
      case TableMapEvent::TYPE_LONG: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint32_t value = context.row_r.read<uint32_t>();
          doc->set<uint64_t>(json_pointer, value);
        } else {
          int32_t value = context.row_r.read<int32_t>();
          doc->set<int64_t>(json_pointer, value);
        }
        break;
      }
      case TableMapEvent::TYPE_LONGLONG: {
        auto unsigned_ = context.signedness_r.read();

        if (unsigned_) {
          uint64_t value = context.row_r.read<uint64_t>();
          doc->set<uint64_t>(json_pointer, value);
        } else {
          int64_t value = context.row_r.read<int64_t>();
          doc->set<int64_t>(json_pointer, value);
        }
        break;
      }
      case TableMapEvent::TYPE_FLOAT:
      case TableMapEvent::TYPE_DOUBLE: {
        context.signedness_r.read();
        uint8_t len = context.column_metatype_r.read<uint8_t>();

        switch (len) {
        case 4: {
          float value = context.row_r.read<float>();
          doc->set(json_pointer, value);
          break;
        }
        case 8: {
          double value = context.row_r.read<double>();
          doc->set(json_pointer, value);
          break;
        }
        }
        break;
      }
      case TableMapEvent::TYPE_BOOL: {
        context.signedness_r.read();

        bool value = context.row_r.read<uint8_t>();
        doc->set(json_pointer, value);
        break;
      }
      case TableMapEvent::TYPE_VARCHAR: {
        uint16_t max_length = context.column_metatype_r.read<uint16_t>();

        uint16_t len = 0;
        if (max_length <= 255) {
          len = context.row_r.read<uint8_t>();
        } else {
          len = context.row_r.read<uint16_t>();
        }
        std::pmr::string str(resource);
        str.resize(len, 0);
        context.row_r.readCpy(str.data(), str.size());

        doc->set(json_pointer, std::move(str));
        break;
      }
      case TableMapEvent::TYPE_STRING: {
        uint8_t real_type = context.column_metatype_r.read<uint8_t>();

        if (real_type != TableMapEvent::TYPE_STRING) {
          goto unexpected_type;
        }
        uint8_t& len = real_type;
        len = context.column_metatype_r.read<uint8_t>() / 4;
        std::pmr::string str(resource);
        str.resize(len, ' ');
        len = context.row_r.read<uint8_t>();
        context.row_r.readCpy(str.data(), len);
        doc->set(json_pointer, std::move(str));
        break;
      }
      default: {
      unexpected_type:
        THROW(OtterBrixDiffSinkError, "Unknown type");
      }
      }
    }
  }
  return doc;
}

std::pair<compare_expression_ptr, parameter_node_ptr>
OtterBrixDiffSink::getSelectionParameters(
    const components::document::document_ptr& doc, ReadContext& context
)
{
  using namespace components::expressions;
  using param_t = core::parameter_id_t;

  if (getPrimaryKeyIndex(context) == -1) {
    THROW(
        OtterBrixDiffSinkError,
        fmt::format(
            "Table {}.{}. Primary key must be one and name == '{}'.",
            context.data.table_name, context.data.collection_name, PK_FIELD_NAME
        )
    );
  }

  auto expr = components::expressions::make_compare_expression(
      resource, compare_type::eq, components::expressions::key_t{PK_FIELD_NAME},
      core::parameter_id_t{1}
  );
  auto params = components::logical_plan::make_parameter_node(resource);

  const auto logic_type = doc->type_by_key(PK_JSON_POINTER);

  assert(logic_type == components::types::logical_type::STRING_LITERAL);

  params->add_parameter(param_t{1}, doc->get_string(PK_JSON_POINTER));

  return {std::move(expr), std::move(params)};
}

int OtterBrixDiffSink::getPrimaryKeyIndex(const ReadContext& context) noexcept
{
  const auto& pk_list = context.data.column_primary_key_list;
  const auto& column_name_list = context.data.column_name_list;

  if (pk_list.size() != 1 || column_name_list[pk_list[0]] != PK_FIELD_NAME) {
    return -1;
  }

  return pk_list[0];
}

OtterBrixConsumerSink::OtterBrixConsumerSink(DataHandler data_handler) :
    OtterBrixConsumerI(data_handler)
{
  const char* path = "/tmp/test_collection_sql/base";
  auto config = configuration::config::create_config(path);
  config.log.level = log_t::level::warn;

  std::filesystem::remove_all(config.main_path);
  std::filesystem::create_directories(config.main_path);

  otterbrix_service = otterbrix::make_otterbrix(config);
}

std::pmr::memory_resource* OtterBrixConsumerSink::resource() const noexcept
{
  return otterbrix_service->dispatcher()->resource();
}

void OtterBrixConsumerSink::putDataImpl(const ExtendedNode& extended_node)
{
  auto node = boost::const_pointer_cast<node_t>(extended_node.node);
  auto params = boost::const_pointer_cast<parameter_node_t>(extended_node.parameter);

  processContextStorage(node);
  assert(otterbrix_service->dispatcher() == otterbrix_service->dispatcher());

  auto res = otterbrix_service->dispatcher()->execute_plan(
      otterbrix::session_id_t(), node, params
  );
  assert(res->is_success());

#ifndef NDEBUG
  selectStage();
#endif
}

void OtterBrixConsumerSink::processContextStorage(node_ptr node)
{
  const auto& database_name = node->database_name();
  const auto& collection_name = node->collection_name();

  auto database_it = context_storage.find(database_name);

  if (database_it == context_storage.end()) {
    otterbrix_service->dispatcher()->create_database(
        otterbrix::session_id_t(), database_name
    );
    database_it = context_storage.emplace(database_name, set_t<std::string>{}).first;
  }

  auto& collection_set = database_it->second;
  auto collection_it = collection_set.find(collection_name);

  if (collection_it == collection_set.end()) {
    otterbrix_service->dispatcher()->create_collection(
        otterbrix::session_id_t(), database_name, collection_name
    );
    collection_it = collection_set.emplace(collection_name).first;
  }
}

void OtterBrixConsumerSink::selectStage()
{
  LOG_INFO() << "++++++++++++++++++++++++++++++++++++++++++++++++";
  for (const auto& [database_name, database] : context_storage) {
    for (const auto& collection_name : database) {
      LOG_INFO() << fmt::format("Collection: `{}`.`{}`", database_name, collection_name);
      auto curson_p = otterbrix_service->dispatcher()->execute_sql(
          otterbrix::session_id_t(),
          fmt::format("SELECT * FROM {}.{};", database_name, collection_name)
      );

      while (curson_p->has_next()) {
        auto data = curson_p->next();
        LOG_INFO() << "    " << data->to_json();
      }

      LOG_INFO() << "+++++++++++++++++++++";
    }
  }
  LOG_INFO() << "++++++++++++++++++++++++++++++++++++++++++++++++";
}
} // namespace cdc