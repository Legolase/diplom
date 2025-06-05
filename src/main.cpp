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
      uptr<cdc::EventSource>(std::move(db_reader_source), [](const cdc::Binlog& ev) {
        using namespace binlog::event;
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

  auto table_diff_source = uptr<cdc::TableDiffSource>(
      std::move(event_source),
      [](const cdc::TableDiff& table_diff) {
      }
  );

  auto otterbrix_consumer =
      uptr<cdc::OtterBrixConsumerSink>([](const cdc::ExtendedNode& e_node) {
        const auto& node = e_node.node;
        const auto& params = e_node.parameter;
        LOG_INFO() << "Node:";

        if (auto node_insert = boost::dynamic_pointer_cast<cdc::node_insert_t>(node);
            node_insert.get())
        {
          LOG_INFO() << "      coll: " << node_insert->collection_full_name().database
                     << '-' << node_insert->collection_full_name().collection;
          LOG_INFO() << "       str: " << node_insert->to_string();
          LOG_INFO() << "       doc: ";
          for (const auto& doc : node_insert->documents()) {
            LOG_INFO() << "            " << doc->to_json();
          }
        } else if (auto node_delete =
                       boost::dynamic_pointer_cast<cdc::node_delete_t>(node);
                   node_delete.get())
        {
          LOG_INFO() << "      coll: " << node_delete->collection_full_name().database
                     << '-' << node_delete->collection_full_name().collection;
          LOG_INFO() << "       str: " << node_delete->to_string();
          LOG_INFO() << "  children: ";
          for (const auto& node_child : node_delete->children()) {
            LOG_INFO() << "            " << node_child->to_string();
          }
          LOG_INFO() << "    params: ";
          for (const auto& it : params->parameters().parameters) {
            LOG_INFO() << "            [" << it.first << "] -> " << it.second.as_string();
          }
        } else if (auto node_update =
                       boost::dynamic_pointer_cast<cdc::node_update_t>(node);
                   node_update.get())
        {
          LOG_INFO() << "      coll: " << node_update->collection_full_name().database
                     << '-' << node_update->collection_full_name().collection;
          LOG_INFO() << "       str: " << node_update->to_string();
          LOG_INFO() << "    update: " << node_update->update()->to_json();
          LOG_INFO() << "  children: ";
          for (const auto& node_child : node_update->children()) {
            LOG_INFO() << "            " << node_child->to_string();
          }
          LOG_INFO() << "    params: ";
          for (const auto& it : params->parameters().parameters) {
            LOG_INFO() << "            [" << it.first << "] -> " << it.second.as_string();
          }
        } else {
          THROW(std::runtime_error, "Unknown node");
        }
      });
  auto otterbrik_diff_sink = uptr<cdc::OtterBrixDiffSink>(
      std::move(otterbrix_consumer), otterbrix_consumer->resource()
  );
  auto main_process = uptr<cdc::MainProcess>(
      std::move(table_diff_source), std::move(otterbrik_diff_sink)
  );

  main_process->process();
}
