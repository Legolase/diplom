#include <array>
#include <gtest/gtest.h>
#include <iostream>

#include <binlog/binlog_events.hpp>
#include <binlog/binlog_reader.hpp>
#include <utils/stream_reader.hpp>
#include <utils/string_buffer_reader.hpp>

TEST(StringBufferReader, Test1) {
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

TEST(StringBufferReader, CheckMoveSemantic1) {
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

TEST(StringBufferReader, CheckMoveSemantic2) {
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

TEST(StringBufferReader, Access) {
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

TEST(StringBufferReader, Sequence1) {
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

TEST(StringBufferReader, Sequence2) {
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

TEST(StringBufferReader, Sequence3) {
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

TEST(StreamReader, All) {
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

TEST(BinlogReader, FormatDescriptionEvent) {
  const char* file_path = "./binlog/mysql-bin.000003";
  std::vector<binlog::event::BinlogEvent::UPtr> v;

  binlog::reader::read(file_path, v);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}