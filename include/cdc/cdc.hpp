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

using node_insert_ptr = components::logical_plan::node_insert_ptr;
using node_delete_ptr = components::logical_plan::node_delete_ptr;
using node_update_ptr = components::logical_plan::node_update_ptr;

using parameter_node_ptr = components::logical_plan::parameter_node_ptr;
using compare_expression_ptr = components::expressions::compare_expression_ptr;

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
  std::variant<node_insert_ptr, node_delete_ptr, node_update_ptr> node_ptr;
  components::logical_plan::parameter_node_ptr parameter;
};

struct DBBufferSource final : BufferSourceI {
  struct Context {};
  DBBufferSource(
      const char* host, const char* user, const char* passwd, const char* db,
      unsigned int port
  );
  virtual ~DBBufferSource();

protected:
  virtual std::optional<Buffer> getDataImpl() final override;

private:
  bool rotate(utils::StringBufferReader& reader);
  void formatDescription(utils::StringBufferReader& reader);

  MYSQL conn;
  MYSQL_RPL rpl;
  binlog::event::FormatDescriptionEvent fde;
  std::optional<std::string> file_path_opt{std::nullopt};
};

struct EventSource final : EventSourceI {
  EventSource(BufferSourceI::UPtr buffer_source, DataHandler event_handler);

protected:
  virtual std::optional<Binlog> getDataImpl() final override;

private:
  BufferSourceI::UPtr buffer_source;
};

struct TableDiffSource final : TableDiffSourceI {
  TableDiffSource(EventSourceI::UPtr event_source, DataHandler table_diff_handler);

protected:
  virtual std::optional<TableDiff> getDataImpl() final override;

private:
  using EventPackage =
      std::pair<binlog::event::TableMapEvent::UPtr, binlog::event::RowsEvent::UPtr>;

  EventPackage getEventPackage();

  EventSourceI::UPtr event_source;
};

struct OtterBrixDiffSink final : OtterBrixDiffSinkI {
  OtterBrixDiffSink(
      OtterBrixConsumerI::UPtr otterbrix_consumer, std::pmr::memory_resource* resource
  );

protected:
  virtual void putDataImpl(const TableDiff& data) final override;

private:
  using node_ptr = components::logical_plan::node_ptr;

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

  enum class FillState {
    OK,
    UNKNOWN_TYPE
  };

  static FillState fillDocument(components::document::document_ptr& doc, ReadContext& context);

  std::pair<compare_expression_ptr, parameter_node_ptr> getSelectionParameters(
      const components::document::document_ptr& doc, ReadContext& context
  );

  OtterBrixConsumerI::UPtr otterbrix_consumer;
  std::pmr::memory_resource* resource;
};

struct OtterBrixConsumerSink final : OtterBrixConsumerI {
  explicit OtterBrixConsumerSink(DataHandler data_handler);

protected:
  virtual void putDataImpl(const ExtendedNode& extended_node) override;
};

} // namespace cdc

#endif