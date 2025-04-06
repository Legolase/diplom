#ifndef _BINLOG_EVENTS_HPP
#define _BINLOG_EVENTS_HPP

#include <binlog/binlog_defines.hpp>

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mysql_binlog::event {

enum class LogEventType {
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

  /*
    The V1 event numbers are used from 5.1.16 until mysql-5.6.
    Not generated since 8.2.0, and rejected by the applier since 8.4.0
  */
  OBSOLETE_WRITE_ROWS_EVENT_V1 = 23,
  OBSOLETE_UPDATE_ROWS_EVENT_V1 = 24,
  OBSOLETE_DELETE_ROWS_EVENT_V1 = 25,

  /**
    Something out of the ordinary happened on the master
   */
  INCIDENT_EVENT = 26,

  /**
    Heartbeat event to be send by master at its idle time
    to ensure master's online status to slave
  */
  HEARTBEAT_LOG_EVENT = 27,

  /**
    In some situations, it is necessary to send over ignorable
    data to the slave: data that a slave can handle in case there
    is code for handling it, but which can be ignored if it is not
    recognized.
  */
  IGNORABLE_LOG_EVENT = 28,
  ROWS_QUERY_LOG_EVENT = 29,

  /** Version 2 of the Row events */
  WRITE_ROWS_EVENT = 30,
  UPDATE_ROWS_EVENT = 31,
  DELETE_ROWS_EVENT = 32,

  GTID_LOG_EVENT = 33,
  ANONYMOUS_GTID_LOG_EVENT = 34,

  PREVIOUS_GTIDS_LOG_EVENT = 35,

  TRANSACTION_CONTEXT_EVENT = 36,

  VIEW_CHANGE_EVENT = 37,

  /* Prepared XA transaction terminal event similar to Xid */
  XA_PREPARE_LOG_EVENT = 38,

  /**
    Extension of UPDATE_ROWS_EVENT, allowing partial values according
    to binlog_row_value_options.
  */
  PARTIAL_UPDATE_ROWS_EVENT = 39,

  TRANSACTION_PAYLOAD_EVENT = 40,

  HEARTBEAT_LOG_EVENT_V2 = 41,

  GTID_TAGGED_LOG_EVENT = 42,
  /**
    Add new events here - right above this comment!
    Existing events (except ENUM_END_EVENT) should never change their numbers
  */
  ENUM_END_EVENT /* end marker */
};

struct EventHeader {
  uint32_t when;
  uint32_t unmasked_server_id;
  uint32_t data_written;
  uint32_t log_pos;
  uint16_t flags;
  uint8_t type_code;
};

struct BinlogEvent {
  using Ptr = std::shared_ptr<BinlogEvent>;

  EventHeader header;
};

struct AppendBlockEvent : BinlogEvent {
  using Ptr = std::shared_ptr<AppendBlockEvent>;

  std::string block;
  uint32_t file_id;
};

struct BeginLoadQueryEvent : AppendBlockEvent {
  using Ptr = std::shared_ptr<BeginLoadQueryEvent>;
};

struct DeleteFileEvent : BinlogEvent {
  using Ptr = std::shared_ptr<DeleteFileEvent>;

  uint32_t file_id;
};

struct FormatDescriptionEvent : BinlogEvent {
  using Ptr = std::shared_ptr<FormatDescriptionEvent>;

  uint32_t created;
  uint16_t binlog_version;
  char server_version[ST_SERVER_VER_LEN];
  bool dont_set_created;
  uint8_t common_header_len;
  std::vector<uint8_t> post_header_len;
  char server_version_split[ST_SERVER_VER_SPLIT_LEN];
  uint8_t number_of_event_types;
};

struct GtidEvent : BinlogEvent {
  using Ptr = std::shared_ptr<GtidEvent>;

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
  std::uint64_t commit_group_ticket{kGroupTicketUnset};

  GtidInfo gtid_info_struct;
  Tsid tsid_parent_struct;
};

struct HeartbeatEvent : BinlogEvent {
  using Ptr = std::shared_ptr<HeartbeatEvent>;

  std::string log_ident;
};

struct HeartbeatV2Event : BinlogEvent {
  using Ptr = std::shared_ptr<HeartbeatV2Event>;

  std::string m_log_filename;
  uint64_t m_log_position;
};

struct IgnorableEvent : BinlogEvent {
  using Ptr = std::shared_ptr<IgnorableEvent>;
};

struct RowsQueryEvent : IgnorableEvent {
  using Ptr = std::shared_ptr<RowsQueryEvent>;

  std::string m_rows_query;
};

struct IncidentEvent : BinlogEvent {
  using Ptr = std::shared_ptr<IncidentEvent>;

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
  using Ptr = std::shared_ptr<IntvarEvent>;

  uint8_t type;
  uint64_t val;
};

struct PreviousGtidEvent : BinlogEvent {
  using Ptr = std::shared_ptr<PreviousGtidEvent>;

  std::string buf;
};

struct QueryEvent : BinlogEvent {
  using Ptr = std::shared_ptr<QueryEvent>;

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
  using Ptr = std::shared_ptr<ExecuteLoadQueryEvent>;

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
  using Ptr = std::shared_ptr<RotateEvent>;

  std::string new_log_ident;
  uint32_t flags;
  uint64_t pos;
};

struct RandEvent : BinlogEvent {
  using Ptr = std::shared_ptr<RandEvent>;

  unsigned long long seed1;
  unsigned long long seed2;
};

struct RowsEvent : BinlogEvent {
  using Ptr = std::shared_ptr<RowsEvent>;

  struct ExtraRowInfo {
    int m_partition_id;
    int m_source_partition_id;
    unsigned char* m_extra_row_ndb_info;
  };

  ExtraRowInfo m_extra_row_info;

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
  using Ptr = std::shared_ptr<DeleteRowsEvent>;
};

struct UpdateRowsEvent : RowsEvent {
  using Ptr = std::shared_ptr<UpdateRowsEvent>;
};

struct WriteRowsEvent : RowsEvent {
  using Ptr = std::shared_ptr<WriteRowsEvent>;
};

struct StopEvent : BinlogEvent {
  using Ptr = std::shared_ptr<StopEvent>;
};

struct TableMapEvent : BinlogEvent {
  using Ptr = std::shared_ptr<TableMapEvent>;

  uint64_t m_table_id;
  uint16_t m_flags;
  size_t m_data_size;
  std::string m_dbnam;
  unsigned long long int m_dblen;
  std::string m_tblnam;
  unsigned long long int m_tbllen;
  std::string m_coltype;
  std::string m_field_metadata;
  std::string m_null_bits;
  std::string m_optional_metadata;
};

struct TransactionContextEvent : BinlogEvent {
  using Ptr = std::shared_ptr<TransactionContextEvent>;

  std::string server_uuid;
  uint32_t thread_id;
  bool gtid_specified;
  const unsigned char* encoded_snapshot_version;
  uint32_t encoded_snapshot_version_length;
  std::list<const char*> write_set;
  std::list<const char*> read_set;
};

struct TransactionPayloadEvent : BinlogEvent {
  using Ptr = std::shared_ptr<TransactionPayloadEvent>;

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
  using Ptr = std::shared_ptr<UnknownEvent>;
};

struct UserVarEvent : BinlogEvent {
  using Ptr = std::shared_ptr<UserVarEvent>;

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
  using Ptr = std::shared_ptr<ViewChangeEvent>;

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
  using Ptr = std::shared_ptr<XAPrepareEvent>;

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
  using Ptr = std::shared_ptr<XidEvent>;

  uint64_t xid;
};

}; // namespace mysql_binlog::event

#endif