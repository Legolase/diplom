#include <binlog/binlog_events.hpp>

#include <cassert>
#include <cstring>
#include <utils.hpp>

#define READ(value, in) (value = stream_utils::read<decltype(value)>(in))
#define READ_ARR(value, in) (stream_utils::memcpy((in), (value), sizeof(value)))

namespace mysql_binlog::event {

EventHeader::EventHeader(LogEventType _type) noexcept :
    when(0),
    data_written(0),
    log_pos(0),
    flags(0),
    type_code(_type) {}

EventHeader::EventHeader(std::istream& in) {
  READ(when, in);
  READ(type_code, in);
  READ(unmasked_server_id, in);
  READ(data_written, in);
  READ(log_pos, in);
  READ(flags, in);
}

BinlogEvent::BinlogEvent(LogEventType _type) :
    header(_type) {}

BinlogEvent::BinlogEvent(std::istream& in) :
    header(in) {}

FormatDescriptionEvent::FormatDescriptionEvent(uint8_t _binlog_ver,
                                               const char* _server_ver) :
    BinlogEvent(LogEventType::FORMAT_DESCRIPTION_EVENT),
    created(0),
    binlog_version(_binlog_ver),
    dont_set_created(false) {
  std::memcpy(server_version, _server_ver, sizeof(server_version));
}

FormatDescriptionEvent::FormatDescriptionEvent(std::istream& in,
                                               FormatDescriptionEvent* fde) :
    BinlogEvent(in) {
  READ(binlog_version, in);
  READ_ARR(server_version, in);
  READ(created, in);
  READ(common_header_len, in);
  
}

} // namespace mysql_binlog::event
