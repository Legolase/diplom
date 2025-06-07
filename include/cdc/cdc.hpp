#ifndef _BUFFER_SOURCE_HPP
#define _BUFFER_SOURCE_HPP

#include <binlog/binlog_events.hpp>
#include <conveyor.hpp>
#include <utils/bit_buffer_reader.hpp>
#include <utils/string_buffer_reader.hpp>

#include <components/document/document.hpp>
#include <components/logical_plan/node_delete.hpp>
#include <components/logical_plan/node_insert.hpp>
#include <components/logical_plan/node_update.hpp>
#include <components/logical_plan/param_storage.hpp>
#include <concepts>
#include <functional>
#include <integration/cpp/otterbrix.hpp>
#include <mysql/mysql.h>
#include <span>
#include <type_traits>
#include <variant>

namespace cdc {

struct TableDiff;

struct ExtendedNode;

using Buffer = std::string_view;
using Binlog = binlog::event::BinlogEvent::UPtr;
using RotateBinlog = binlog::event::RotateEvent::UPtr;
using BufferSourceI = conveyor::Source<Buffer>;
using EventSourceI = conveyor::Source<Binlog>;
using TableDiffSourceI = conveyor::Source<TableDiff>;
using OtterBrixDiffSinkI = conveyor::Sink<TableDiff>;
using OtterBrixConsumerI = conveyor::Sink<ExtendedNode>;
using MainProcess = conveyor::Universal<TableDiff>;

using node_t = components::logical_plan::node_t;
using node_ptr = components::logical_plan::node_ptr;
using node_insert_t = components::logical_plan::node_insert_t;
using node_insert_ptr = components::logical_plan::node_insert_ptr;
using node_delete_t = components::logical_plan::node_delete_t;
using node_delete_ptr = components::logical_plan::node_delete_ptr;
using node_update_t = components::logical_plan::node_update_t;
using node_update_ptr = components::logical_plan::node_update_ptr;

using parameter_node_t = components::logical_plan::parameter_node_t;
using parameter_node_ptr = components::logical_plan::parameter_node_ptr;
using compare_expression_ptr = components::expressions::compare_expression_ptr;

template<typename K, typename V>
using map_t = std::unordered_map<K, V>;

template<typename T>
using set_t = std::unordered_set<T>;

struct TableDiff {
  enum Type {
    INSERT,
    DELETE,
    UPDATE
  } type;

  std::string collection_name;
  std::string table_name;
  std::string column_types;
  std::string column_metatypes;
  std::vector<std::string> column_name_list;
  std::vector<uint16_t> column_primary_key_list;
  std::string column_signedness;
  std::vector<uint8_t> row;
  int64_t width{0};
};

struct ExtendedNode {
  node_ptr node;
  components::logical_plan::parameter_node_ptr parameter;
};

struct DBBufferSource final : BufferSourceI {

  DECLARE_EXCEPTION(DBConnectionError);
  DECLARE_EXCEPTION(DBBinlogError);

  DBBufferSource(
      const char* host, const char* user, const char* passwd, const char* db,
      unsigned int port
  );
  virtual ~DBBufferSource();

protected:
  virtual std::optional<Buffer> getDataImpl() final override;

private:
  void connect();
  void disconnect();
  void rotate();

  std::string_view nextEventBuffer();
  void process(std::string_view buffer);

  MYSQL conn;
  MYSQL_RPL rpl;
  binlog::event::FormatDescriptionEvent fde{
      binlog::BINLOG_VERSION, binlog::SERVER_VERSION
  };
  const std::string host;
  const std::string user;
  const std::string passwd;
  const std::string db;
  const int port;
  std::string file_path;
  uint32_t next_pos{4};
};

struct EventSource final : EventSourceI {
  EventSource(BufferSourceI::UPtr buffer_source, DataHandler event_handler);

protected:
  virtual std::optional<Binlog> getDataImpl() final override;

private:
  BufferSourceI::UPtr buffer_source;
};

struct TableDiffSource final : TableDiffSourceI {

  DECLARE_EXCEPTION(TableDiffSourceError);

  TableDiffSource(EventSourceI::UPtr event_source, DataHandler table_diff_handler);

protected:
  virtual std::optional<TableDiff> getDataImpl() final override;

private:
  using TablePtr = binlog::event::TableMapEvent::UPtr;
  using RowsPtr = binlog::event::RowsEvent::UPtr;
  using EventPackage = std::pair<TablePtr, RowsPtr>;

  EventPackage getEventPackage();

  void submitTableInfo(TablePtr&& tm_event);
  TablePtr extractTableInfo(const uint64_t table_id);

  EventSourceI::UPtr event_source;
  map_t<uint64_t, binlog::event::TableMapEvent::UPtr> table_info_map;
};

struct OtterBrixDiffSink final : OtterBrixDiffSinkI {

  DECLARE_EXCEPTION(OtterBrixDiffSinkError);

  OtterBrixDiffSink(
      OtterBrixConsumerI::UPtr otterbrix_consumer, std::pmr::memory_resource* resource
  );

protected:
  virtual void putDataImpl(const TableDiff& data) final override;

private:
  using node_ptr = components::logical_plan::node_ptr;

  static const std::string PK_FIELD_NAME;
  static const std::string PK_JSON_POINTER;

  struct ReadContext {
    explicit ReadContext(const TableDiff& data);

    utils::StringBufferReader column_metatype_r;
    utils::StringBufferReader column_type_r;
    utils::StringBufferReader row_r;
    utils::BitBufferReader<utils::BitOrder::BIG_END> signedness_r;
    const TableDiff& data;

    struct CachedData {
      std::vector<bool> null_bitmap;
      std::string json_pointer{[]() {
        std::string json_pointer;
        json_pointer.reserve(64);
        return json_pointer;
      }()};
    } cached_data;
  };

  void sendNodesInsert(const TableDiff& data);
  void sendNodesDelete(const TableDiff& data);
  void sendNodesUpdate(const TableDiff& data);

  components::document::document_ptr getDocument(ReadContext& context);

  std::pair<compare_expression_ptr, parameter_node_ptr> getSelectionParameters(
      const components::document::document_ptr& doc, ReadContext& context
  );

  /**
   * @brief Returns the index of the column that is the primary key.
   *
   * @param[in] context Read context information
   *
   * @returns The index of the primary key or '-1' if the primary keys of the table are
   * invalid.
   */
  static int getPrimaryKeyIndex(const ReadContext& context) noexcept;

  OtterBrixConsumerI::UPtr otterbrix_consumer;
  std::pmr::memory_resource* resource;
};

struct OtterBrixConsumerSink : OtterBrixConsumerI {
  explicit OtterBrixConsumerSink(DataHandler data_handler);

  std::pmr::memory_resource* resource() const noexcept;

protected:
  virtual void putDataImpl(const ExtendedNode& extended_node) override;

  void processContextStorage(node_ptr node);
  void selectStage();

  otterbrix::otterbrix_ptr otterbrix_service;
  map_t<std::string, set_t<std::string>> context_storage;
};

} // namespace cdc

#endif