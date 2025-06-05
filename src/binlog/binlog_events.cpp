#include <binlog/binlog_events.hpp>

#include <cassert>
#include <cstring>

#define READ(value) (value = reader.read<decltype(value)>())
#define READ_ARR(dest, size) (reader.readCpy(reinterpret_cast<char*>(dest), size))
#define PEEK(value, ...) (value = reader.peek<decltype(value)>(__VA_ARGS__))
#define PEEK_ARR(dest, ...) (reader.peekCpy(reinterpret_cast<char*>(dest), __VA_ARGS__))

namespace {

int64_t get_packed_integer(utils::StringBufferReader& reader)
{
  uint64_t result = 0;
  READ_ARR(&result, 1);

  if (result < 251) {
  } else if (result == 251) {
    return (unsigned long)~0;
  } else if (result == 252) {
    READ_ARR(&result, 2);
  } else if (result == 253) {
    READ_ARR(&result, 3);
  } else {
    READ_ARR(&result, 8);
  }
  return result;
}

uint64_t get_server_version_value(const char* p)
{
  char* r;
  uint64_t number;
  uint64_t result = 0;
  for (unsigned int i = 0; i <= 2; i++) {
    number = strtoul(p, &r, 10);
    if (number < 256 && (*r == '.' || i != 0))
      result = (result * 256) + number;
    else {
      result = 0;
      break;
    }

    p = r;
    if (*r == '.') p++;
  }

  return result;
}
} // namespace

namespace binlog::event {

EventHeader::EventHeader(LogEventType _type) noexcept :
    when(0),
    data_written(0),
    log_pos(0),
    flags(0),
    type_code(_type)
{}

EventHeader::EventHeader(utils::StringBufferReader& reader)
{
  READ(when);
  READ(type_code);
  READ(unmasked_server_id);
  READ(data_written);
  READ(log_pos);
  READ(flags);
}

BinlogEvent::BinlogEvent(LogEventType _type) :
    header(_type)
{}

BinlogEvent::BinlogEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde) :
    header(reader)
{
  if (header.type_code != LogEventType::FORMAT_DESCRIPTION_EVENT) {
    if (fde->has_checksum) {
      reader.flipEnd(CHECKSUM_CRC32_SIGNATURE_LEN);
    }
  }
}

void BinlogEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << " Header:";
  LOG_INFO(out) << "                 when: " << header.when;
  LOG_INFO(out) << "   unmasked_server_id: " << header.unmasked_server_id;
  LOG_INFO(out) << "         data_written: " << header.data_written;
  LOG_INFO(out) << "              log_pos: " << header.log_pos;
  LOG_INFO(out) << "                flags: " << header.flags;
  LOG_INFO(out) << "            type_code: " << static_cast<int>(header.type_code);
}

FormatDescriptionEvent::FormatDescriptionEvent(
    uint8_t _binlog_ver, const char* _server_ver
) :
    BinlogEvent(LogEventType::FORMAT_DESCRIPTION_EVENT),
    created(0),
    binlog_version(_binlog_ver),
    dont_set_created(false)
{
  const auto _server_ver_size = std::strlen(_server_ver);
  std::memset(server_version, 0, sizeof(server_version));
  std::memcpy(server_version, _server_ver, _server_ver_size);

  common_header_len = LOG_EVENT_HEADER_LEN;
  post_header_len.resize(LOG_EVENT_TYPES);

#ifndef NDEBUG
  // Using to set check than all events size were initialized
  std::memset(post_header_len.data(), 255, post_header_len.size());
#endif

  post_header_len[START_EVENT_V3 - 1] = START_V3_HEADER_LEN;
  post_header_len[QUERY_EVENT - 1] = QUERY_HEADER_LEN;
  post_header_len[STOP_EVENT - 1] = STOP_HEADER_LEN;
  post_header_len[ROTATE_EVENT - 1] = ROTATE_HEADER_LEN;
  post_header_len[INTVAR_EVENT - 1] = INTVAR_HEADER_LEN;
  post_header_len[LOAD_EVENT - 1] = LOAD_HEADER_LEN;
  post_header_len[SLAVE_EVENT - 1] = SLAVE_HEADER_LEN;
  post_header_len[CREATE_FILE_EVENT - 1] = CREATE_FILE_HEADER_LEN;
  post_header_len[APPEND_BLOCK_EVENT - 1] = APPEND_BLOCK_HEADER_LEN;
  post_header_len[EXEC_LOAD_EVENT - 1] = EXEC_LOAD_HEADER_LEN;
  post_header_len[DELETE_FILE_EVENT - 1] = DELETE_FILE_HEADER_LEN;
  post_header_len[NEW_LOAD_EVENT - 1] = NEW_LOAD_HEADER_LEN;
  post_header_len[RAND_EVENT - 1] = RAND_HEADER_LEN;
  post_header_len[USER_VAR_EVENT - 1] = USER_VAR_HEADER_LEN;
  post_header_len[FORMAT_DESCRIPTION_EVENT - 1] = FORMAT_DESCRIPTION_HEADER_LEN;
  post_header_len[XID_EVENT - 1] = XID_HEADER_LEN;
  post_header_len[XA_PREPARE_LOG_EVENT - 1] = XA_PREPARE_HEADER_LEN;
  post_header_len[BEGIN_LOAD_QUERY_EVENT - 1] = BEGIN_LOAD_QUERY_HEADER_LEN;
  post_header_len[EXECUTE_LOAD_QUERY_EVENT - 1] = EXECUTE_LOAD_QUERY_HEADER_LEN;
  post_header_len[PRE_GA_WRITE_ROWS_EVENT - 1] = 0;
  post_header_len[PRE_GA_UPDATE_ROWS_EVENT - 1] = 0;
  post_header_len[PRE_GA_DELETE_ROWS_EVENT - 1] = 0;
  post_header_len[TABLE_MAP_EVENT - 1] = TABLE_MAP_HEADER_LEN;
  post_header_len[WRITE_ROWS_EVENT_V1 - 1] = ROWS_HEADER_LEN_V1;
  post_header_len[UPDATE_ROWS_EVENT_V1 - 1] = ROWS_HEADER_LEN_V1;
  post_header_len[DELETE_ROWS_EVENT_V1 - 1] = ROWS_HEADER_LEN_V1;
  post_header_len[INCIDENT_EVENT - 1] = INCIDENT_HEADER_LEN;
  post_header_len[HEARTBEAT_LOG_EVENT - 1] = 0;
  post_header_len[IGNORABLE_LOG_EVENT - 1] = 0;
  post_header_len[ROWS_QUERY_LOG_EVENT - 1] = 0;
  post_header_len[GTID_LOG_EVENT - 1] = 0;
  post_header_len[ANONYMOUS_GTID_LOG_EVENT - 1] = 0;
  post_header_len[PREVIOUS_GTIDS_LOG_EVENT - 1] = 0;
  post_header_len[TRANSACTION_CONTEXT_EVENT - 1] = 0;
  post_header_len[VIEW_CHANGE_EVENT - 1] = 0;
  post_header_len[XA_PREPARE_LOG_EVENT - 1] = 0;
  post_header_len[PARTIAL_UPDATE_ROWS_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[TRANSACTION_PAYLOAD_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[HEARTBEAT_LOG_EVENT_V2 - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[WRITE_ROWS_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[UPDATE_ROWS_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[DELETE_ROWS_EVENT - 1] = ROWS_HEADER_LEN_V2;
  std::memset(
      post_header_len.data() + MYSQL_EVENTS_END - 1, 0,
      MARIA_EVENTS_BEGIN - MYSQL_EVENTS_END
  );
  post_header_len[ANNOTATE_ROWS_EVENT - 1] = ANNOTATE_ROWS_HEADER_LEN;
  post_header_len[BINLOG_CHECKPOINT_EVENT - 1] = BINLOG_CHECKPOINT_HEADER_LEN;
  post_header_len[GTID_EVENT - 1] = GTID_HEADER_LEN;
  post_header_len[GTID_LIST_EVENT - 1] = GTID_LIST_HEADER_LEN;
  post_header_len[START_ENCRYPTION_EVENT - 1] = START_ENCRYPTION_HEADER_LEN;
  post_header_len[QUERY_COMPRESSED_EVENT - 1] = QUERY_HEADER_LEN;
  post_header_len[WRITE_ROWS_COMPRESSED_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[UPDATE_ROWS_COMPRESSED_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[DELETE_ROWS_COMPRESSED_EVENT - 1] = ROWS_HEADER_LEN_V2;
  post_header_len[WRITE_ROWS_COMPRESSED_EVENT_V1 - 1] = ROWS_HEADER_LEN_V1;
  post_header_len[UPDATE_ROWS_COMPRESSED_EVENT_V1 - 1] = ROWS_HEADER_LEN_V1;
  post_header_len[DELETE_ROWS_COMPRESSED_EVENT_V1 - 1] = ROWS_HEADER_LEN_V1;
#ifndef NDEBUG
  for (const auto& elem : post_header_len) {
    if (elem == 255) {
      THROW(std::runtime_error, "Not all post-header size elements were initialized");
    }
  }
#endif
}

FormatDescriptionEvent::FormatDescriptionEvent(
    utils::StringBufferReader& reader, FormatDescriptionEvent* fde
) :
    BinlogEvent(reader, fde),
    common_header_len(0)
{
  size_t number_of_event_types = 0;

  READ(binlog_version);
  READ_ARR(server_version, sizeof(server_version));
  server_version[sizeof(server_version) - 1] = 0;
  READ(created);
  dont_set_created = true;
  READ(common_header_len);

  if (common_header_len < LOG_EVENT_HEADER_LEN) {
    THROW(std::runtime_error, "Invalid Format_description common header length");
  }

  size_t available_bytes = reader.available();

  if (server_version_value() >= checksum_version_product) {
    available_bytes -= BINLOG_CHECKSUM_ALG_DESC_LEN + CHECKSUM_CRC32_SIGNATURE_LEN;
    has_checksum = true;
  }
  number_of_event_types = available_bytes;

  post_header_len.resize(number_of_event_types);
  READ_ARR(post_header_len.data(), post_header_len.size());
}

void FormatDescriptionEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "FormatDescriptionEvent: ";
  BinlogEvent::show(out);
  LOG_INFO(out) << " Other info:";
  LOG_INFO(out) << "             created: " << created;
  LOG_INFO(out) << "      binlog_version: " << binlog_version;
  LOG_INFO(out) << "      server_version: " << server_version;
  LOG_INFO(out) << "    dont_set_created: " << dont_set_created;
  LOG_INFO(out) << "   common_header_len: " << static_cast<int>(common_header_len);
  LOG_INFO(out) << "     post_header_len: " << post_header_len;
  LOG_INFO(out) << "        has_checksum: " << has_checksum;
}

uint64_t FormatDescriptionEvent::server_version_value() const
{
  return get_server_version_value(server_version);
}

RotateEvent::RotateEvent(std::string _new_log_ident, uint32_t _flags, uint64_t _pos) :
    BinlogEvent(LogEventType::ROTATE_EVENT),
    new_log_ident(std::move(_new_log_ident)),
    flags(_flags),
    pos(_pos)
{}

RotateEvent::RotateEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde) :
    BinlogEvent(reader, fde)
{
  uint8_t post_header_size = fde->post_header_len[LogEventType::ROTATE_EVENT - 1];
  flags = DUPNAME;
  if (post_header_size) {
    READ(pos);
  } else {
    pos = 4;
  }
  size_t avail_to_read = reader.available();
  if (avail_to_read == 0) {
    THROW(std::runtime_error, "The event is too short");
  }

  if (avail_to_read > ROTATE_EVENT_MAX_FULL_NAME_SIZE - 1) {
    avail_to_read = ROTATE_EVENT_MAX_FULL_NAME_SIZE - 1;
  }

  new_log_ident.resize(avail_to_read);

  READ_ARR(new_log_ident.data(), avail_to_read);
}

void RotateEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "RotateEvent: ";
  BinlogEvent::show(out);
  LOG_INFO(out) << " Other info:";
  LOG_INFO(out) << "   new_log_ident: " << new_log_ident;
  LOG_INFO(out) << "           flags: " << flags;
  LOG_INFO(out) << "             pos: " << pos;
}

RowsEvent::RowsEvent(LogEventType type) :
    BinlogEvent(type),
    m_table_id(0),
    m_width(0),
    columns_before_image(0),
    columns_after_image(0),
    row(0)
{}

RowsEvent::RowsEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde) :
    BinlogEvent(reader, fde)
{
  LogEventType type = header.type_code;
  const auto post_header_len = fde->post_header_len[(int)type - 1];
  m_type = type;

  m_table_id = 0;
  if (post_header_len != 6) {
    READ_ARR(&m_table_id, 6);
  } else {
    READ_ARR(&m_table_id, 4);
  }
  READ(m_flags);

  /* Rows header len v2*/
  if (post_header_len == 10) {
    READ(var_header_len);
    var_header_len -= 2;

    /* Skips the extra_rows_info */
    reader.skip(var_header_len);
  }

  m_width = get_packed_integer(reader);

  if (m_width == 0) {
    THROW(std::runtime_error, "Invalid length of RowsEvent size");
  }

  n_bits_len = (m_width + 7) / 8;

  columns_before_image.resize(n_bits_len);

  READ_ARR(columns_before_image.data(), columns_before_image.size());

  if (header.type_code == LogEventType::UPDATE_ROWS_EVENT ||
      header.type_code == LogEventType::UPDATE_ROWS_EVENT_V1 ||
      header.type_code == LogEventType::PARTIAL_UPDATE_ROWS_EVENT)
  {
    columns_after_image.resize(n_bits_len);

    READ_ARR(columns_after_image.data(), columns_after_image.size());
  } else {
    columns_after_image = columns_before_image;
  }

  size_t data_size = reader.available();

  row.resize(data_size);
  READ_ARR(row.data(), row.size());
}

void RowsEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "RowsEvent: ";
  BinlogEvent::show(out);
  LOG_INFO(out) << " Other info:";
  LOG_INFO(out) << "                 m_type: " << static_cast<int>(m_type);
  LOG_INFO(out) << "             m_table_id: " << m_table_id;
  LOG_INFO(out) << "                m_flags: " << m_flags;
  LOG_INFO(out) << "                m_width: " << m_width;
  LOG_INFO(out) << "             n_bits_len: " << n_bits_len;
  LOG_INFO(out) << "         var_header_len: " << var_header_len;
  LOG_INFO(out) << "   columns_before_image: " << columns_before_image;
  LOG_INFO(out) << "    columns_after_image: " << columns_after_image;
  LOG_INFO(out) << "                    row: " << row;
}

DeleteRowsEvent::DeleteRowsEvent(
    utils::StringBufferReader& reader, FormatDescriptionEvent* fde
) :
    RowsEvent(reader, fde)
{
  header.type_code = m_type;
}

void DeleteRowsEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "DeleteRowsEvent: ";
  RowsEvent::show(out);
}

UpdateRowsEvent::UpdateRowsEvent(
    utils::StringBufferReader& reader, FormatDescriptionEvent* fde
) :
    RowsEvent(reader, fde)
{
  header.type_code = m_type;
}

void UpdateRowsEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "UpdateRowsEvent: ";
  RowsEvent::show(out);
}

WriteRowsEvent::WriteRowsEvent(
    utils::StringBufferReader& reader, FormatDescriptionEvent* fde
) :
    RowsEvent(reader, fde)
{
  header.type_code = m_type;
}

void WriteRowsEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "WriteRowsEvent: ";
  RowsEvent::show(out);
}

TableMapEvent::TableMapEvent(
    utils::StringBufferReader& reader, FormatDescriptionEvent* fde
) :
    BinlogEvent(reader, fde)
{
  m_table_id = 0;
  if (fde->post_header_len[LogEventType::TABLE_MAP_EVENT - 1] == 6) {
    READ_ARR(&m_table_id, 4);
  } else {
    READ_ARR(&m_table_id, 6);
  }
  READ(m_flags);

  uint64_t db_name_len = get_packed_integer(reader) + 1;

  m_dbnam.resize(db_name_len);
  READ_ARR(m_dbnam.data(), m_dbnam.size());
  m_dbnam.pop_back(); // remove '\0'

  uint64_t tb_name_len = get_packed_integer(reader) + 1;

  m_tblnam.resize(tb_name_len);
  READ_ARR(m_tblnam.data(), m_tblnam.size());
  m_tblnam.pop_back(); // remove '\0'

  column_count = get_packed_integer(reader);

  m_coltype.resize(column_count);
  READ_ARR(m_coltype.data(), m_coltype.size());

  if (reader.available() > 0) {
    const auto meta_data_size = get_packed_integer(reader);
    if (meta_data_size > (column_count * 4)) {
      THROW(std::runtime_error, "Invalid size of meta_data part");
    }
    const auto null_bytes_count = (column_count + 7) / 8;
    m_field_metadata.resize(meta_data_size);
    m_null_bits.resize(null_bytes_count);
    READ_ARR(m_field_metadata.data(), m_field_metadata.size());
    READ_ARR(m_null_bits.data(), m_null_bits.size());
  }

  const auto optional_len = reader.available();
  if (optional_len > 0) {
    m_optional_metadata.resize(optional_len);
    READ_ARR(m_optional_metadata.data(), m_optional_metadata.size());
  }
}

void TableMapEvent::show(std::ostream& out) const
{
  LOG_INFO(out) << "TableMapEvent: ";
  BinlogEvent::show(out);
  LOG_INFO(out) << " Other info:";
  LOG_INFO(out) << "            m_table_id: " << m_table_id;
  LOG_INFO(out) << "               m_flags: " << m_flags;
  LOG_INFO(out) << "           m_data_size: " << m_data_size;
  LOG_INFO(out) << "          column_count: " << column_count;
  LOG_INFO(out) << "               m_dbnam: " << m_dbnam;
  LOG_INFO(out) << "              m_tblnam: " << m_tblnam;
  LOG_INFO(out) << "             m_coltype: " << std::string_view(m_coltype);
  LOG_INFO(out) << "      m_field_metadata: " << std::string_view(m_field_metadata);
  LOG_INFO(out) << "           m_null_bits: " << std::string_view(m_null_bits);
  LOG_INFO(out) << "   m_optional_metadata: " << std::string_view(m_optional_metadata);
}

std::vector<uint16_t> TableMapEvent::getSimplePrimaryKey() const
{
  const auto opt_simple_pk = getOptionalField(SIMPLE_PRIMARY_KEY);

  if (!opt_simple_pk.has_value()) {
    return {};
  }

  const auto& simple_pk = opt_simple_pk.value();
  utils::StringBufferReader reader(simple_pk.data(), simple_pk.size());
  std::vector<uint16_t> result;

  while (reader.available()) {
    result.push_back(get_packed_integer(reader));
  }

  return result;
}

std::vector<std::string> TableMapEvent::getColumnName() const
{
  auto opt_column_name = getOptionalField(OptinalMetadataType::COLUMN_NAME);

  if (!opt_column_name.has_value()) {
    return {};
  }

  const auto& column_name = opt_column_name.value();
  utils::StringBufferReader reader(column_name.data(), column_name.size());
  std::vector<std::string> result;

  while (reader.available()) {
    size_t column_name_length = get_packed_integer(reader);
    result.emplace_back(reader.ptr(), column_name_length);
    reader.skip(column_name_length);
  }

  return result;
}

std::string TableMapEvent::getSignedness() const
{
  auto opt_signedness = getOptionalField(OptinalMetadataType::SIGNEDNESS);

  if (!opt_signedness.has_value()) {
    return {};
  }

  const auto& signedness = opt_signedness.value();
  return std::string(signedness.data(), signedness.size());
}

const std::optional<std::span<char>>
TableMapEvent::getOptionalField(OptinalMetadataType needed_type) const noexcept
{
  utils::StringBufferReader reader(
      m_optional_metadata.data(), m_optional_metadata.size()
  );

  while (reader.available()) {
    OptinalMetadataType metadata_type;

    READ(metadata_type);
    uint64_t metadata_length = get_packed_integer(reader);

    if (metadata_type != needed_type) {
      reader.skip(metadata_length);
    } else {
      return std::span<char>(const_cast<char*>(reader.ptr()), metadata_length);
    }
  }
  return std::nullopt;
}

} // namespace binlog::event
