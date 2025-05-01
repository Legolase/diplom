#ifndef _BINLOG_EVENTS_HPP
#define _BINLOG_EVENTS_HPP

#include <binlog/binlog_defines.hpp>
#include <utils/string_buffer_reader.hpp>

#include <cstdint>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace mysql_binlog::event {

enum LogEventType : uint8_t {
  /**
    Every time you add a type, you have to
    - Assign it a number explicitly. Otherwise it will cause trouble
      if a event type before is deprecated and removed directly from
      the enum.
    - Fix Format_description_event::Format_description_event().
  */
  UNKNOWN_EVENT = 0,
  /*
    Deprecated since mysql 8.0.2. It is just a placeholder,
    should not be used anywhere else.
  */
  START_EVENT_V3 = 1,
  QUERY_EVENT = 2,
  STOP_EVENT = 3,
  ROTATE_EVENT = 4,
  INTVAR_EVENT = 5,

  SLAVE_EVENT = 7,

  APPEND_BLOCK_EVENT = 9,
  DELETE_FILE_EVENT = 11,

  RAND_EVENT = 13,
  USER_VAR_EVENT = 14,
  FORMAT_DESCRIPTION_EVENT = 15,
  XID_EVENT = 16,
  BEGIN_LOAD_QUERY_EVENT = 17,
  EXECUTE_LOAD_QUERY_EVENT = 18,

  TABLE_MAP_EVENT = 19,

  OBSOLETE_WRITE_ROWS_EVENT_V1 = 23,
  OBSOLETE_UPDATE_ROWS_EVENT_V1 = 24,
  OBSOLETE_DELETE_ROWS_EVENT_V1 = 25,

  INCIDENT_EVENT = 26,

  HEARTBEAT_LOG_EVENT = 27,

  IGNORABLE_LOG_EVENT = 28,
  ROWS_QUERY_LOG_EVENT = 29,

  WRITE_ROWS_EVENT = 30,
  UPDATE_ROWS_EVENT = 31,
  DELETE_ROWS_EVENT = 32,

  GTID_LOG_EVENT = 33,
  ANONYMOUS_GTID_LOG_EVENT = 34,

  PREVIOUS_GTIDS_LOG_EVENT = 35,

  TRANSACTION_CONTEXT_EVENT = 36,

  VIEW_CHANGE_EVENT = 37,

  XA_PREPARE_LOG_EVENT = 38,

  PARTIAL_UPDATE_ROWS_EVENT = 39,

  TRANSACTION_PAYLOAD_EVENT = 40,

  HEARTBEAT_LOG_EVENT_V2 = 41,

  GTID_TAGGED_LOG_EVENT = 42,
  ENUM_END_EVENT
};

enum LogEventPostHeaderSize : uint32_t {
  // where 3.23, 4.x and 5.0 agree
  QUERY_HEADER_MINIMAL_LEN = (4 + 4 + 1 + 2),
  // where 5.0 differs: 2 for length of N-bytes vars.
  QUERY_HEADER_LEN = (QUERY_HEADER_MINIMAL_LEN + 2),
  STOP_HEADER_LEN = 0,
  START_V3_HEADER_LEN = (2 + ST_SERVER_VER_LEN + 4),
  // this is FROZEN (the Rotate post-header is frozen)
  ROTATE_HEADER_LEN = 8,
  INTVAR_HEADER_LEN = 0,
  APPEND_BLOCK_HEADER_LEN = 4,
  DELETE_FILE_HEADER_LEN = 4,
  RAND_HEADER_LEN = 0,
  USER_VAR_HEADER_LEN = 0,
  FORMAT_DESCRIPTION_HEADER_LEN = (START_V3_HEADER_LEN + 1 + ENUM_END_EVENT - 1),
  XID_HEADER_LEN = 0,
  BEGIN_LOAD_QUERY_HEADER_LEN = APPEND_BLOCK_HEADER_LEN,
  ROWS_HEADER_LEN_V1 = 8,
  TABLE_MAP_HEADER_LEN = 8,
  EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN = (4 + 4 + 4 + 1),
  EXECUTE_LOAD_QUERY_HEADER_LEN =
      (QUERY_HEADER_LEN + EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN),
  INCIDENT_HEADER_LEN = 2,
  HEARTBEAT_HEADER_LEN = 0,
  IGNORABLE_HEADER_LEN = 0,
  ROWS_HEADER_LEN_V2 = 10,
  TRANSACTION_CONTEXT_HEADER_LEN = 18,
  VIEW_CHANGE_HEADER_LEN = 52,
  XA_PREPARE_HEADER_LEN = 0,
  TRANSACTION_PAYLOAD_HEADER_LEN = 0
};

enum class ChecksumAlg {
  OFF = 0,
  CRC32 = 1,
  ENUM_END,
  UNDEF = 255
};

inline constexpr int LOG_EVENT_TYPES = LogEventType::ENUM_END_EVENT - 1;

struct FormatDescriptionEvent;

struct EventHeader {
  explicit EventHeader(LogEventType _type = LogEventType::ENUM_END_EVENT) noexcept;
  explicit EventHeader(utils::StringBufferReader& reader);

  uint32_t when;
  uint32_t unmasked_server_id;
  uint32_t data_written;
  uint32_t log_pos;
  uint16_t flags;
  LogEventType type_code;
};

struct BinlogEvent {
  using UPtr = std::unique_ptr<BinlogEvent>;
  using SPtr = std::shared_ptr<BinlogEvent>;

  explicit BinlogEvent(LogEventType _type);
  explicit BinlogEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;

  EventHeader header;
};

struct AppendBlockEvent : BinlogEvent {
  using UPtr = std::unique_ptr<AppendBlockEvent>;
  using SPtr = std::shared_ptr<AppendBlockEvent>;

  std::string block;
  uint32_t file_id;
};

struct BeginLoadQueryEvent : AppendBlockEvent {
  using UPtr = std::unique_ptr<BeginLoadQueryEvent>;
  using SPtr = std::shared_ptr<BeginLoadQueryEvent>;
};

struct DeleteFileEvent : BinlogEvent {
  using UPtr = std::unique_ptr<DeleteFileEvent>;
  using SPtr = std::shared_ptr<DeleteFileEvent>;

  uint32_t file_id;
};

struct FormatDescriptionEvent : BinlogEvent {
  using UPtr = std::unique_ptr<FormatDescriptionEvent>;
  using SPtr = std::shared_ptr<FormatDescriptionEvent>;

  FormatDescriptionEvent(uint8_t binlog_ver, const char* server_ver);
  FormatDescriptionEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;

  uint64_t server_version_value() const;

  uint32_t created;
  uint16_t binlog_version;
  char server_version[ST_SERVER_VER_LEN];
  bool dont_set_created;
  uint8_t common_header_len;
  std::vector<uint8_t> post_header_len;
  bool has_checksum{true};
};

struct GtidEvent : BinlogEvent {
protected:
  static constexpr int ENCODED_FLAG_LENGTH = 1;
  static constexpr int ENCODED_SID_LENGTH = 16;
  static constexpr int ENCODED_GNO_LENGTH = 8;
  static constexpr int LOGICAL_TIMESTAMP_TYPECODE_LENGTH = 1;
  static constexpr int LOGICAL_TIMESTAMP_LENGTH = 16;

public:
  using UPtr = std::unique_ptr<GtidEvent>;
  using SPtr = std::shared_ptr<GtidEvent>;

  static constexpr uint64_t kGroupTicketUnset = 0;

  static constexpr int POST_HEADER_LENGTH =
      ENCODED_FLAG_LENGTH + ENCODED_SID_LENGTH + ENCODED_GNO_LENGTH +
      LOGICAL_TIMESTAMP_TYPECODE_LENGTH + LOGICAL_TIMESTAMP_LENGTH;

  struct GtidInfo {
    int32_t rpl_gtid_sidno;
    int64_t rpl_gtid_gno;
  };

  struct Tsid {
    Uuid m_uuid = {0};
    Tag m_tag;
  };

  int64_t last_committed;
  int64_t sequence_number;
  unsigned const char FLAG_MAY_HAVE_SBR = 1;
  bool may_have_sbr_stmts;
  unsigned char gtid_flags = 0;
  uint64_t original_commit_timestamp;
  uint64_t immediate_commit_timestamp;
  bool has_commit_timestamps;
  uint64_t transaction_length;
  uint32_t original_server_version;
  uint32_t immediate_server_version;
  uint64_t commit_group_ticket{kGroupTicketUnset};

  GtidInfo gtid_info_struct;
  Tsid tsid_parent_struct;
};

struct HeartbeatEvent : BinlogEvent {
  using UPtr = std::unique_ptr<HeartbeatEvent>;
  using SPtr = std::shared_ptr<HeartbeatEvent>;

  std::string log_ident;
};

struct HeartbeatV2Event : BinlogEvent {
  using UPtr = std::unique_ptr<HeartbeatV2Event>;

  std::string m_log_filename;
  uint64_t m_log_position;
};

struct IgnorableEvent : BinlogEvent {
  using UPtr = std::unique_ptr<IgnorableEvent>;
  using SPtr = std::shared_ptr<IgnorableEvent>;
};

struct RowsQueryEvent : IgnorableEvent {
  using UPtr = std::unique_ptr<RowsQueryEvent>;
  using SPtr = std::shared_ptr<RowsQueryEvent>;

  std::string m_rows_query;
};

struct IncidentEvent : BinlogEvent {
  using UPtr = std::unique_ptr<IncidentEvent>;
  using SPtr = std::shared_ptr<IncidentEvent>;

  enum class EnumIncident {
    /** No incident */
    INCIDENT_NONE = 0,
    /** There are possibly lost events in the replication stream */
    INCIDENT_LOST_EVENTS = 1,
    /** Shall be last event of the enumeration */
    INCIDENT_COUNT
  };

  EnumIncident incident;
  std::string message;
};

struct IntvarEvent : BinlogEvent {
  using UPtr = std::unique_ptr<IntvarEvent>;
  using SPtr = std::shared_ptr<IntvarEvent>;

  uint8_t type;
  uint64_t val;
};

struct PreviousGtidEvent : BinlogEvent {
  using UPtr = std::unique_ptr<PreviousGtidEvent>;
  using SPtr = std::shared_ptr<PreviousGtidEvent>;

  std::string buf;
};

struct QueryEvent : BinlogEvent {
  using UPtr = std::unique_ptr<QueryEvent>;
  using SPtr = std::shared_ptr<QueryEvent>;

  enum class Ternary {
    TERNARY_UNSET,
    TERNARY_OFF,
    TERNARY_ON
  };

  std::string query;
  std::string db;
  std::string catalog;
  std::string time_zone_str;
  uint32_t thread_id;
  uint32_t query_exec_time;
  uint16_t error_code;
  uint16_t status_vars_len;
  bool flags2_inited;
  bool sql_mode_inited;
  bool charset_inited;
  uint32_t flags2;
  uint64_t sql_mode;
  uint16_t auto_increment_increment;
  uint16_t auto_increment_offset;
  char charset[6];
  uint16_t lc_time_names_number;
  uint16_t charset_database_number;
  uint64_t table_map_for_update;
  Ternary explicit_defaults_ts;
  unsigned char mts_accessed_dbs;
  char mts_accessed_db_names[MAX_DBS_IN_EVENT_MTS][NAME_LEN];
  uint64_t ddl_xid;
  uint16_t default_collation_for_utf8mb4_number;
  uint8_t sql_require_primary_key;
  uint8_t default_table_encryption;

  std::string user;
  std::string host;
  unsigned long data_len;
};

struct ExecuteLoadQueryEvent : QueryEvent {
  using UPtr = std::unique_ptr<ExecuteLoadQueryEvent>;
  using SPtr = std::shared_ptr<ExecuteLoadQueryEvent>;

  enum class LoadDupHandling {
    LOAD_DUP_ERROR = 0,
    LOAD_DUP_IGNORE,
    LOAD_DUP_REPLACE
  };

  int32_t file_id; /** file_id of temporary file */
  uint32_t
      fn_pos_start;    /** pointer to the part of the query that should be substituted */
  uint32_t fn_pos_end; /** pointer to the end of this part of query */
  LoadDupHandling dup_handling;
};

struct RotateEvent : BinlogEvent {
  using UPtr = std::unique_ptr<RotateEvent>;
  using SPtr = std::shared_ptr<RotateEvent>;

  static constexpr uint32_t DUPNAME = 2;
  static constexpr uint32_t RELOG = 4;

  RotateEvent(std::string _new_log_ident, uint32_t _flags, uint64_t _pos);
  RotateEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;

  std::string new_log_ident;
  uint32_t flags;
  uint64_t pos;
};

struct RandEvent : BinlogEvent {
  using UPtr = std::unique_ptr<RandEvent>;
  using SPtr = std::shared_ptr<RandEvent>;

  unsigned long long seed1;
  unsigned long long seed2;
};

struct RowsEvent : BinlogEvent {
  using UPtr = std::unique_ptr<RowsEvent>;
  using SPtr = std::shared_ptr<RowsEvent>;

  struct ExtraRowInfo {
    int m_partition_id;
    int m_source_partition_id;
    unsigned char* m_extra_row_ndb_info;
  };

  explicit RowsEvent(LogEventType type);
  RowsEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;

  /// #TO_CHECK: If the info needed for CDC?
  // ExtraRowInfo m_extra_row_info;

  LogEventType m_type;
  uint64_t m_table_id;
  uint16_t m_flags;
  unsigned long m_width;
  uint32_t n_bits_len;
  uint16_t var_header_len;
  std::vector<uint8_t> columns_before_image;
  std::vector<uint8_t> columns_after_image;
  std::vector<uint8_t> row;
};

struct DeleteRowsEvent : RowsEvent {
  using UPtr = std::unique_ptr<DeleteRowsEvent>;
  using SPtr = std::shared_ptr<DeleteRowsEvent>;

  DeleteRowsEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;
};

struct UpdateRowsEvent : RowsEvent {
  using UPtr = std::unique_ptr<UpdateRowsEvent>;
  using SPtr = std::shared_ptr<UpdateRowsEvent>;

  UpdateRowsEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;
};

struct WriteRowsEvent : RowsEvent {
  using UPtr = std::unique_ptr<WriteRowsEvent>;
  using SPtr = std::shared_ptr<WriteRowsEvent>;

  WriteRowsEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;
};

struct StopEvent : BinlogEvent {
  using UPtr = std::unique_ptr<StopEvent>;
  using SPtr = std::shared_ptr<StopEvent>;
};

struct TableMapEvent : BinlogEvent {
  using UPtr = std::unique_ptr<TableMapEvent>;
  using SPtr = std::shared_ptr<TableMapEvent>;

  TableMapEvent(utils::StringBufferReader& reader, FormatDescriptionEvent* fde);

  void show(std::ostream& out = std::cout) const;

  uint64_t m_table_id{0};
  uint16_t m_flags{0};
  size_t m_data_size{0};
  int64_t column_count{0};
  std::string m_dbnam;
  std::string m_tblnam;
  std::string m_coltype;
  std::string m_field_metadata;
  std::string m_null_bits;
  std::string m_optional_metadata;
};

struct TransactionContextEvent : BinlogEvent {
  using UPtr = std::unique_ptr<TransactionContextEvent>;
  using SPtr = std::shared_ptr<TransactionContextEvent>;

  std::string server_uuid;
  uint32_t thread_id;
  bool gtid_specified;
  const unsigned char* encoded_snapshot_version;
  uint32_t encoded_snapshot_version_length;
  std::list<const char*> write_set;
  std::list<const char*> read_set;
};

struct TransactionPayloadEvent : BinlogEvent {
  using UPtr = std::unique_ptr<TransactionPayloadEvent>;
  using SPtr = std::shared_ptr<TransactionPayloadEvent>;

  enum class CompressionType {
    ZSTD,
    NONE
  };

  std::string m_payload;
  std::string m_buffer_sequence_view;
  uint64_t m_payload_size{0};
  CompressionType m_compression_type;
  uint64_t m_uncompressed_size{0};
};

struct UnknownEvent : BinlogEvent {
  using UPtr = std::unique_ptr<UnknownEvent>;
  using SPtr = std::shared_ptr<UnknownEvent>;
};

struct UserVarEvent : BinlogEvent {
  using UPtr = std::unique_ptr<UserVarEvent>;
  using SPtr = std::shared_ptr<UserVarEvent>;

  enum class ValueType {
    INVALID_RESULT = -1, /* not valid for UDFs */
    STRING_RESULT = 0,   /* char * */
    REAL_RESULT,         /* double */
    INT_RESULT,          /* long long */
    ROW_RESULT,          /* not valid for UDFs */
    DECIMAL_RESULT       /* char *, to be converted to/from a decimal */
  };

  std::string name;
  std::string val;
  ValueType type;
  unsigned int charset_number;
  bool is_null;
  unsigned char flags;
};

struct ViewChangeEvent : BinlogEvent {
  using UPtr = std::unique_ptr<ViewChangeEvent>;
  using SPtr = std::shared_ptr<ViewChangeEvent>;

  static const int ENCODED_VIEW_ID_OFFSET = 0;
  static const int ENCODED_SEQ_NUMBER_OFFSET = 40;
  static const int ENCODED_CERT_INFO_SIZE_OFFSET = 48;
  static const int ENCODED_VIEW_ID_MAX_LEN = 40;
  static const int ENCODED_CERT_INFO_KEY_SIZE_LEN = 2;
  static const int ENCODED_CERT_INFO_VALUE_LEN = 4;

  char view_id[ENCODED_VIEW_ID_MAX_LEN];
  long long int seq_number;
  std::unordered_map<std::string, std::string> certification_info;
};

struct XAPrepareEvent : BinlogEvent {
  using UPtr = std::unique_ptr<XAPrepareEvent>;
  using SPtr = std::shared_ptr<XAPrepareEvent>;

  static const int MY_XIDDATASIZE = 128;

  struct MY_XID {
    long formatID;
    long gtrid_length;
    long bqual_length;
    char data[MY_XIDDATASIZE];
  };

  MY_XID my_xid;
  void* xid;
  bool one_phase;
};

struct XidEvent : BinlogEvent {
  using UPtr = std::unique_ptr<XidEvent>;
  using SPtr = std::shared_ptr<XidEvent>;

  uint64_t xid;
};

}; // namespace mysql_binlog::event

#endif