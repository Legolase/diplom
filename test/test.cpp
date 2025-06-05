#include <array>
#include <gtest/gtest.h>
#include <iostream>

#include <binlog/binlog_events.hpp>
#include <binlog/binlog_reader.hpp>
#include <utils/stream_reader.hpp>
#include <utils/string_buffer_reader.hpp>

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
      "\x00\x00\x00\x00\x00\x01\x00\x07\x65\x5f\x73\x74\x6f\x72\x65\x00\x06\x62\x72\x61"
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

  EXPECT_EQ(tm_event.m_table_id, 18UL);
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
  EXPECT_EQ(std::memcmp(signedness.data(),"\x80", 1), 0);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}