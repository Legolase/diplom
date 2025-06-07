#include <binlog/binlog_events.hpp>
#include <binlog/binlog_reader.hpp>
#include <cdc/cdc.hpp>
#include <utils/stream_reader.hpp>
#include <utils/string_buffer_reader.hpp>

#include <array>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

#define READ(reader, value) ((value) = reader.read<decltype(value)>())
#define PEEK(reader, value, ...) ((value) = reader.peek<decltype(value)>(__VA_ARGS__))

TEST(StringBufferReader, Test1)
{
  using namespace utils;

  const char* input_line = "\xf0\x00\x00\x00\x01\x00\x00\x00";
  StringBufferReader reader(input_line, 8);

  int a, b;

  a = reader.peek<int>();
  b = reader.read<int>();

  ASSERT_EQ(a, b);
  ASSERT_EQ(a, 240);
  ASSERT_EQ(b, 240);
  a = reader.peek<int>();
  ASSERT_NE(a, b);
  ASSERT_EQ(a, 1);
  b = reader.read<int>();
  ASSERT_EQ(a, b);
  ASSERT_EQ(b, 1);

  try {
    // Check that even 1 byte can't be read
    b = reader.read<char>();
    FAIL() << "The expected exception did not occur";
  } catch (BadStream& e) {
  }
}

TEST(StringBufferReader, CheckMoveSemantic1)
{
  using namespace utils;

  const char* input_line = "\xf0\x00\x00\x00\x01\x00\x00\x00";
  std::string str(input_line, 8);
  StringBufferReader reader(str.data(), str.size());

  int a, b;

  a = reader.peek<int>();
  str[2] = '\x0a';
  b = reader.peek<int>();

  ASSERT_NE(a, b);
  ASSERT_EQ(a, 240);
  ASSERT_EQ(b, 655600);
  a = reader.peek<int>(4);
  ASSERT_EQ(a, 1);
  str[6] = '\xff';
  a = reader.peek<int>(4);
  ASSERT_EQ(a, 16711681);
}

TEST(StringBufferReader, CheckMoveSemantic2)
{
  using namespace utils;

  const char* input_line = "\xf0\x00\x00\x00\x01\x00\x00\x00";
  std::string_view str_view(input_line, 8);
  StringBufferReader reader(str_view);

  int a, b;

  a = reader.read<int>();
  b = reader.read<int>();

  ASSERT_NE(a, b);
  ASSERT_EQ(a, 240);
  ASSERT_EQ(b, 1);
}

TEST(StringBufferReader, Access)
{
  using namespace utils;

  const char* empty_line = "";
  const char* byte_line = "\x10";
  const char* word_line = "\x0b\x7a";

  // Check for 0 byte line
  {
    StringBufferReader reader(empty_line, 0);
    try {
      int a = reader.read<char>();
      FAIL() << "Expected exception. Unexpected success read of empty string";
    } catch (BadStream& e) {
    }
  }
  // Check for 1 byte line
  {
    StringBufferReader reader(byte_line, 1);
    try {
      int a = reader.read<char>();
      ASSERT_EQ(a, 16);
    } catch (BadStream& e) {
      FAIL() << "Unexpected exception. Expected success read of 1 char string";
    }
    try {
      int b = reader.read<char>();
      FAIL() << "Expected exception. Unexpected success read of empty tail";
    } catch (BadStream& e) {
    }
  }
  // Check for 2 bytes line
  {
    StringBufferReader reader(word_line, 2);
    int a, b;
    try {
      a = reader.read<char>();
      ASSERT_EQ(a, 11);
    } catch (BadStream& e) {
      FAIL() << "Unexpected exception. Expected success read of 1 char";
    }
    try {
      b = reader.peek<char>();
      ASSERT_EQ(b, 122);
    } catch (BadStream& e) {
      FAIL() << "Unexpected exception. Expected success read of 1 char";
    }
    try {
      b = reader.peek<uint16_t>();
      FAIL() << "Expected exception. Unexpected success read of 2 char";
    } catch (BadStream& e) {
    }
  }
}

TEST(StringBufferReader, Sequence1)
{
  using namespace utils;

  char input_line[] = "\x01 \x20\x03 \x07\x80\x09\xa0";

  StringBufferReader reader(input_line, sizeof(input_line) - 1);
  uint32_t a, b, c;
  a = reader.peek<char>();
  b = reader.peek<uint16_t>(2);
  c = reader.peek<uint32_t>(5);

  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 800);
  ASSERT_EQ(c, 2684977159);
}

TEST(StringBufferReader, Sequence2)
{
  using namespace utils;

  char input_line[] = "\x01 \x20\x03 \x07\x80\x09\xa0";

  StringBufferReader reader(input_line, sizeof(input_line) - 1);
  std::string a = "-";
  std::string b = "--";
  std::string c = "----";
  reader.peekCpy(a.data(), 0, a.size());
  reader.peekCpy(b.data(), 2, b.size());
  reader.peekCpy(c.data(), 5, c.size());

  ASSERT_EQ(a, "\x01");
  ASSERT_EQ(b, "\x20\x03");
  ASSERT_EQ(c, "\x07\x80\x09\xa0");
}

TEST(StringBufferReader, Sequence3)
{
  using namespace utils;

  char input_line[] = "\x01 \x20\x03 \x07\x80\x09\xa0 \x0b";

  StringBufferReader reader(input_line, sizeof(input_line) - 1);
  std::string a = "-";
  std::string b = "--";
  std::string c = "----";
  reader.readCpy(a.data(), a.size());
  reader.read<char>();
  reader.readCpy(b.data(), b.size());
  reader.read<char>();
  reader.readCpy(c.data(), c.size());

  ASSERT_EQ(a, "\x01");
  ASSERT_EQ(b, "\x20\x03");
  ASSERT_EQ(c, "\x07\x80\x09\xa0");

  try {
    reader.readCpy(c.data(), c.size());
    FAIL() << "Expected exception out of stream buffer reading";
  } catch (BadStream& e) {
  }
}

TEST(StreamReader, All)
{
  using namespace utils;
  const char input_line[] = "\x01\xf0\x02\xe0\x03\xd0\x04\xc0";
  const size_t input_line_size = sizeof(input_line) - 1;

  std::istringstream stream(std::string(input_line, input_line_size));
  StreamReader reader(stream);

  uint32_t a;
  unsigned char b;
  uint64_t c;

  a = reader.peek<int>();
  b = reader.peek<char>();
  c = reader.peek<uint64_t>();

  EXPECT_EQ(a, 3758288897);
  EXPECT_EQ(b, 1);
  b = reader.peek<char>(1);
  EXPECT_EQ(b, 0xf0);
  EXPECT_EQ(c, 13836412670250774529ull);

  b = reader.read<char>();

  EXPECT_EQ(b, 1);

  a = reader.peek<int>();
  b = reader.peek<char>();

  EXPECT_EQ(a, 0x3e002f0);
  EXPECT_EQ(b, 0xf0);
  try {
    c = reader.peek<uint64_t>();
    FAIL() << "Expected expection. Out of stream end";
  } catch (BadStream& e) {
    EXPECT_EQ(c, 13836412670250774529ull);
  }
}

TEST(BinlogReader, FormatDescriptionEvent)
{
  binlog::event::FormatDescriptionEvent fde_start(
      binlog::BINLOG_VERSION, binlog::SERVER_VERSION
  );
  const char fde_buffer[] =
      "\xc9\xe4\x41\x68\x0f\x01\x00\x00\x00\xfc\x00\x00\x00\x00\x01\x00\x00\x00\x00\x04"
      "\x00\x31\x31\x2e\x34\x2e\x35\x2d\x4d\x61\x72\x69\x61\x44\x42\x2d\x75\x62\x75\x32"
      "\x34\x30\x34\x2d\x6c\x6f\x67\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xc9\xe4\x41\x68\x13\x38\x0d\x00\x08"
      "\x00\x12\x00\x04\x04\x04\x04\x12\x00\x00\xe4\x00\x04\x1a\x08\x00\x00\x00\x08\x08"
      "\x08\x02\x00\x00\x00\x0a\x0a\x0a\x00\x00\x00\x00\x00\x00\x0a\x0a\x0a\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x13\x04\x00"
      "\x0d\x08\x08\x08\x0a\x0a\x0a\x00\xbf\xf4\xea\xc1";
  const auto fde_size = sizeof(fde_buffer) - 1;
  utils::StringBufferReader reader(fde_buffer, fde_size);
  std::vector<uint8_t> expected_post_header_len = {
      0x38, 0x0d, 0x00, 0x08, 0x00, 0x12, 0x00, 0x04, 0x04, 0x04, 0x04, 0x12, 0x00, 0x00,
      0xe4, 0x00, 0x04, 0x1a, 0x08, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x02, 0x00, 0x00,
      0x00, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x13, 0x04, 0x00, 0x0d, 0x08, 0x08, 0x08,
      0x0a, 0x0a, 0x0a
  };

  binlog::event::FormatDescriptionEvent fde_event(reader, &fde_start);

  EXPECT_EQ(fde_event.header.when, 1749148873UL);
  EXPECT_EQ(fde_event.header.type_code, 15UL);
  EXPECT_EQ(fde_event.header.unmasked_server_id, 1UL);
  EXPECT_EQ(fde_event.header.data_written, 252UL);
  EXPECT_EQ(fde_event.header.log_pos, 256UL);
  EXPECT_EQ(fde_event.header.flags, 0UL);

  EXPECT_EQ(fde_event.binlog_version, 4UL);
  EXPECT_TRUE(
      std::memcmp(
          fde_event.server_version,
          "\x31\x31\x2e\x34\x2e\x35\x2d\x4d\x61\x72\x69\x61\x44\x42\x2d\x75\x62\x75\x32"
          "\x34\x30\x34\x2d\x6c\x6f\x67\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
          "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
          sizeof(fde_event.server_version)
      ) == 0
  );
  EXPECT_TRUE(fde_event.dont_set_created);
  EXPECT_EQ(fde_event.created, 1749148873UL);
  EXPECT_EQ(fde_event.common_header_len, 19UL);
  EXPECT_EQ(fde_event.post_header_len, expected_post_header_len);
  EXPECT_TRUE(fde_event.has_checksum);
}

TEST(BinlogReader, RotateEvent)
{
  binlog::event::FormatDescriptionEvent fde_start(
      binlog::BINLOG_VERSION, binlog::SERVER_VERSION
  );
  const char rotate_buffer[] =
      "\x01\x01\x01\x01\x04\x05\x00\x00\x00\x2b\x00\x00\x00\x01\x20\x03\x40\x20\x00\x04"
      "\x00\x00\x01\x00\x00\x00\x00\x6d\x79\x73\x71\x6c\x2d\x62\x69\x6e\x2e\x30\x30\x30"
      "\x31\x32\x31";
  const auto rotate_size = sizeof(rotate_buffer) - 1;
  utils::StringBufferReader reader(rotate_buffer, rotate_size);

  binlog::event::RotateEvent rotate_event(reader, &fde_start);

  EXPECT_EQ(rotate_event.header.when, 16843009UL);
  EXPECT_EQ(rotate_event.header.type_code, 4UL);
  EXPECT_EQ(rotate_event.header.unmasked_server_id, 5UL);
  EXPECT_EQ(rotate_event.header.data_written, 43UL);
  EXPECT_EQ(rotate_event.header.log_pos, 1073946625UL);
  EXPECT_EQ(rotate_event.header.flags, 32UL);

  EXPECT_EQ(rotate_event.flags, 2UL);
  EXPECT_EQ(rotate_event.pos, 16777220UL);
  EXPECT_EQ(rotate_event.new_log_ident, "mysql-bin.000121");
}

TEST(BinlogReader, TableMapEvent)
{
  binlog::event::FormatDescriptionEvent fde_start(
      binlog::BINLOG_VERSION, binlog::SERVER_VERSION
  );

  const char tm_buffer[] =
      "\x59\x0e\x42\x68\x13\x01\x0a\x00\x00\x49\x00\x00\x00\x00\x00\x00\x00\x00\x00\x12"
      "\x00\x00\x00\x00\xff\x01\x00\x07\x65\x5f\x73\x74\x6f\x72\x65\x00\x06\x62\x72\x61"
      "\x6e\x64\x73\x00\x02\x08\x0f\x02\xe8\x03\x00\x01\x01\x80\x02\x03\xfc\x00\x09\x04"
      "\x09\x03\x5f\x69\x64\x04\x6e\x61\x6d\x65\x08\x01\x00";
  const auto tm_size = sizeof(tm_buffer) - 1;
  utils::StringBufferReader reader(tm_buffer, tm_size);

  binlog::event::TableMapEvent tm_event(reader, &fde_start);

  EXPECT_EQ(tm_event.header.when, 1749159513UL);
  EXPECT_EQ(tm_event.header.type_code, 19UL);
  EXPECT_EQ(tm_event.header.unmasked_server_id, 2561UL);
  EXPECT_EQ(tm_event.header.data_written, 73UL);
  EXPECT_EQ(tm_event.header.log_pos, 0UL);
  EXPECT_EQ(tm_event.header.flags, 0UL);

  EXPECT_EQ(tm_event.m_table_id, 280375465082898UL);
  EXPECT_EQ(tm_event.m_flags, 1UL);
  EXPECT_EQ(tm_event.m_dbnam, "e_store");
  EXPECT_EQ(tm_event.m_tblnam, "brands");
  EXPECT_EQ(tm_event.column_count, 2UL);
  EXPECT_EQ(std::memcmp(tm_event.m_coltype.data(), "\x08\x0f", 2), 0);
  EXPECT_EQ(std::memcmp(tm_event.m_field_metadata.data(), "\xE8\x03", 2), 0);
  EXPECT_EQ(std::memcmp(tm_event.m_null_bits.data(), "\x00", 1), 0);

  const char expected_optional_metadata[] =
      "\x01\x01\x80\x02\x03\xfc\x00\x09\x04\x09\x03\x5f\x69\x64\x04\x6e\x61\x6d\x65\x08"
      "\x01\x00";
  EXPECT_EQ(
      std::memcmp(
          tm_event.m_optional_metadata.data(), expected_optional_metadata,
          sizeof(expected_optional_metadata) - 1
      ),
      0
  );

  const auto simple_pk = tm_event.getSimplePrimaryKey();
  EXPECT_EQ(simple_pk, std::vector<uint16_t>{0});

  const auto column_name_list = tm_event.getColumnName();
  std::vector<std::string> expected_column_name_list;
  expected_column_name_list.push_back("_id");
  expected_column_name_list.push_back("name");
  EXPECT_EQ(column_name_list, expected_column_name_list);

  const auto signedness = tm_event.getSignedness();
  EXPECT_EQ(std::memcmp(signedness.data(), "\x80", 1), 0);
}

TEST(BinlogReader, WriteRowsEvent)
{
  binlog::event::FormatDescriptionEvent fde_start(
      binlog::BINLOG_VERSION, binlog::SERVER_VERSION
  );

  const char wr_buffer[] =
      "\x59\x0e\x42\x68\x17\x01\x0a\x0b\x0c\x2f\x00\x00\x00\x05\x00\x00\x00\x00\x00\x12"
      "\x0A\x01\x02\x03\x0f\x01\x00\x02\x03\xfc\x01\x00\x00\x00\x00\x00\x00\x00\x07\x00"
      "\x53\x61\x6d\x73\x75\x6e\x67";
  const auto wr_size = sizeof(wr_buffer) - 1;
  utils::StringBufferReader reader(wr_buffer, wr_size);

  binlog::event::WriteRowsEvent wr_event(reader, &fde_start);

  EXPECT_EQ(wr_event.header.when, 1749159513UL);
  EXPECT_EQ(wr_event.header.type_code, 23UL);
  EXPECT_EQ(wr_event.header.unmasked_server_id, 202050049UL);
  EXPECT_EQ(wr_event.header.data_written, 47UL);
  EXPECT_EQ(wr_event.header.log_pos, 5UL);
  EXPECT_EQ(wr_event.header.flags, 0UL);

  EXPECT_EQ(wr_event.m_type, wr_event.header.type_code);
  EXPECT_EQ(wr_event.m_table_id, 16505592941074UL);
  EXPECT_EQ(wr_event.m_flags, 1UL);
  EXPECT_EQ(wr_event.m_width, 2UL);
  EXPECT_EQ(wr_event.n_bits_len, 1);
  EXPECT_EQ(std::memcmp(wr_event.columns_before_image.data(), "\x03", 1), 0);
  EXPECT_EQ(std::memcmp(wr_event.columns_after_image.data(), "\x03", 1), 0);

  const char expected_row[] =
      "\xfc\x01\x00\x00\x00\x00\x00\x00\x00\x07\x00\x53\x61\x6d\x73\x75\x6e\x67\x00";
  EXPECT_EQ(std::memcmp(wr_event.row.data(), expected_row, sizeof(expected_row) - 1), 0);
}

namespace cdc {
struct TestBufferSource final : BufferSourceI {

  DECLARE_EXCEPTION(TestBufferSourceError);

  TestBufferSource(const char* buffer, size_t buffer_size) noexcept :
      reader(buffer, buffer_size)
  {}
  virtual ~TestBufferSource() noexcept {}

protected:
  virtual std::optional<Buffer> getDataImpl() final override
  {
    try {
      uint32_t event_size;
      PEEK(reader, event_size, binlog::DATA_WRITTEN_OFFSET);

      if (event_size > reader.available()) {
        THROW(TestBufferSourceError, "Event size more than available bytes to read");
      }

      Buffer result(reader.ptr(), event_size);
      reader.skip(event_size);

      return result;
    } catch (utils::BadStream& e) {
      LOG_WARNING() << e.what();

      if (reader.available() != 0) {
        THROW(
            TestBufferSourceError,
            "Not end of stream, but not enouth bytes to read size of event"
        );
      }

      return std::nullopt;
    }
  }

private:
  utils::StringBufferReader reader;
};

struct TestOtterBrixConsumerSink final : OtterBrixConsumerSink {
  TestOtterBrixConsumerSink() :
      OtterBrixConsumerSink([](const ExtendedNode&) {
      })
  {}

  void testFinalState()
  {
    auto cur = otterbrix_service->dispatcher()->execute_sql(
        otterbrix::session_id_t(), "SELECT * FROM e_store.table;"
    );

    ASSERT_EQ(cur->size(), 6);

    while (cur->has_next()) {
      const auto doc = cur->next();
      const auto _id = std::stoi(doc->get_string("/_id").c_str());

      switch (_id) {
      case 1: {
        EXPECT_EQ(doc->get_string("/big_varchar"), "minimal");
        EXPECT_EQ(doc->get_int("/bool"), 0);
        EXPECT_EQ(doc->get_string("/char"), "char_min        ");
        EXPECT_EQ(doc->get_double("/double"), -1.7e+308);
        EXPECT_EQ(doc->get_long("/s_bigint"), -9223372036854775808L);
        EXPECT_EQ(doc->get_int("/s_int"), -2147483648);
        EXPECT_EQ(doc->get_string("/small_varchar"), "min");
        EXPECT_EQ(doc->get_int("/s_medium"), -8388608);
        EXPECT_EQ(doc->get_smallint("/s_smallint"), -32768);
        EXPECT_EQ(doc->get_tinyint("/s_tinyint"), -128);
        break;
      }
      case 2: {
        EXPECT_EQ(doc->get_string("/big_varchar"), "maximal value test string");
        EXPECT_EQ(doc->get_int("/bool"), 1);
        EXPECT_EQ(doc->get_string("/char"), "char_max        ");
        EXPECT_EQ(doc->get_double("/double"), 1.7e+308);
        EXPECT_EQ(doc->get_long("/s_bigint"), 9223372036854775807);
        EXPECT_EQ(doc->get_int("/s_int"), 2147483647);
        EXPECT_EQ(doc->get_string("/small_varchar"), "max");
        EXPECT_EQ(doc->get_int("/s_medium"), 8388607);
        EXPECT_EQ(doc->get_smallint("/s_smallint"), 32767);
        EXPECT_EQ(doc->get_tinyint("/s_tinyint"), 127);
        break;
      }
      case 3: {
        EXPECT_EQ(doc->get_string("/big_varchar"), "");
        EXPECT_EQ(doc->get_int("/bool"), 0);
        EXPECT_EQ(doc->get_string("/char"), "                ");
        EXPECT_EQ(doc->get_double("/double"), 0.L);
        EXPECT_EQ(doc->get_long("/s_bigint"), 0);
        EXPECT_EQ(doc->get_int("/s_int"), 0);
        EXPECT_EQ(doc->get_string("/small_varchar"), "");
        EXPECT_EQ(doc->get_int("/s_medium"), 0);
        EXPECT_EQ(doc->get_smallint("/s_smallint"), 0);
        EXPECT_EQ(doc->get_tinyint("/s_tinyint"), 0);
        break;
      }
      case 4: {
        EXPECT_EQ(doc->get_string("/big_varchar"), "negative 1");
        EXPECT_EQ(doc->get_int("/bool"), 1);
        EXPECT_EQ(doc->get_string("/char"), "neg             ");
        EXPECT_EQ(doc->get_double("/double"), 0.5L);
        EXPECT_EQ(doc->get_long("/s_bigint"), -1);
        EXPECT_EQ(doc->get_int("/s_int"), -1);
        EXPECT_EQ(doc->get_string("/small_varchar"), "neg1");
        EXPECT_EQ(doc->get_int("/s_medium"), -1);
        EXPECT_EQ(doc->get_smallint("/s_smallint"), -1);
        EXPECT_EQ(doc->get_tinyint("/s_tinyint"), -1);
        break;
      }
      case 6: {
        EXPECT_EQ(doc->get_string("/big_varchar"), "b");
        EXPECT_EQ(doc->get_int("/bool"), 0);
        EXPECT_EQ(doc->get_string("/char"), "c               ");
        EXPECT_EQ(doc->get_double("/double"), 0.12345);
        EXPECT_EQ(doc->get_long("/s_bigint"), 900);
        EXPECT_EQ(doc->get_int("/s_int"), 700);
        EXPECT_EQ(doc->get_string("/small_varchar"), "upd");
        EXPECT_EQ(doc->get_int("/s_medium"), 500);
        EXPECT_EQ(doc->get_smallint("/s_smallint"), 300);
        EXPECT_EQ(doc->get_tinyint("/s_tinyint"), 100);
        break;
      }
      case 7: {
        EXPECT_EQ(doc->get_string("/big_varchar"), "arbitrary data");
        EXPECT_EQ(doc->get_int("/bool"), 1);
        EXPECT_EQ(doc->get_string("/char"), "test_string     ");
        EXPECT_EQ(doc->get_double("/double"), 2.718281828);
        EXPECT_EQ(doc->get_long("/s_bigint"), 1234567890123456);
        EXPECT_EQ(doc->get_int("/s_int"), 1000000);
        EXPECT_EQ(doc->get_string("/small_varchar"), "test");
        EXPECT_EQ(doc->get_int("/s_medium"), 123456);
        EXPECT_EQ(doc->get_smallint("/s_smallint"), 1024);
        EXPECT_EQ(doc->get_tinyint("/s_tinyint"), 42);
        break;
      }
      }
    }
  }
};
} // namespace cdc

template<typename T, typename... Args>
decltype(auto) uptr(Args&&... args)
{
  return std::make_unique<T>(std::forward<Args>(args)...);
}

std::string getFileData(const char* file_path)
{
  std::ifstream in(file_path, std::ios::in | std::ios::binary);

  if (!in) {
    THROW(std::runtime_error, fmt::format("Can't open file '{}'", file_path));
  }

  const auto file_start = in.tellg();
  in.seekg(0, std::ios::end);
  const auto file_end = in.tellg();
  in.seekg(file_start);

  std::string result(file_end - file_start, 0);

  const auto real_read = in.readsome(result.data(), result.size());

  if (real_read != result.size()) {
    THROW(std::logic_error, fmt::format("Can't read the whole file"));
  }
  const char* buffer = result.data();
  const auto size_ = result.size();
  return result;
}

TEST(ChangeDataCapture, Convertion)
{
  const auto events_buffer = getFileData("../../static/binlog/test2.bin");
  cdc::BufferSourceI::UPtr buffer_source =
      std::make_unique<cdc::TestBufferSource>(events_buffer.data(), events_buffer.size());
  auto event_source =
      uptr<cdc::EventSource>(std::move(buffer_source), [](const cdc::Binlog& ev) {
      });

  auto table_diff_source = uptr<cdc::TableDiffSource>(
      std::move(event_source),
      [](const cdc::TableDiff& table_diff) {
      }
  );

  auto otterbrix_consumer = uptr<cdc::TestOtterBrixConsumerSink>();
  auto* otterbrix_consumer_raw_ptr = otterbrix_consumer.get();
  auto otterbrik_diff_sink = uptr<cdc::OtterBrixDiffSink>(
      std::move(otterbrix_consumer), otterbrix_consumer->resource()
  );
  auto main_process = uptr<cdc::MainProcess>(
      std::move(table_diff_source), std::move(otterbrik_diff_sink)
  );

  main_process->process();

  ASSERT_NE(otterbrix_consumer_raw_ptr, nullptr);

  otterbrix_consumer_raw_ptr->testFinalState();
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}