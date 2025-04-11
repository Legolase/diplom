#include <gtest/gtest.h>
#include <iostream>

#include <string_buffer_reader.hpp>

TEST(StringBufferReader, Test1) {
  using namespace utils;

  const char* input_line = "\xf0\x00\x00\x00\x01\x00\x00\x00";
  StringBufferReader reader(input_line, 8);

  int a, b;

  a = reader.fetch<int>();
  b = reader.read<int>();

  ASSERT_EQ(a, b);
  ASSERT_EQ(a, 240);
  ASSERT_EQ(b, 240);
  a = reader.fetch<int>();
  ASSERT_NE(a, b);
  ASSERT_EQ(a, 1);
  b = reader.read<int>();
  ASSERT_EQ(a, b);
  ASSERT_EQ(b, 1);

  try {
    // Check that even 1 byte can't be read
    b = reader.read<char>();
    FAIL() << "The expected exception did not occur";
  } catch (StringBufferReader::BadStream& e) {
  }
}

TEST(StringBufferReader, CheckMoveSemantic1) {
  using namespace utils;

  const char* input_line = "\xf0\x00\x00\x00\x01\x00\x00\x00";
  std::string str(input_line, 8);
  StringBufferReader reader(str.data(), str.size());

  int a, b;

  a = reader.fetch<int>();
  str[2] = '\x0a';
  b = reader.fetch<int>();

  ASSERT_NE(a, b);
  ASSERT_EQ(a, 240);
  ASSERT_EQ(b, 655600);
  a = reader.fetch<int>(4);
  ASSERT_EQ(a, 1);
  str[6] = '\xff';
  a = reader.fetch<int>(4);
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
    } catch (StringBufferReader::BadStream& e) {
    }
  }
  // Check for 1 byte line
  {
    StringBufferReader reader(byte_line, 1);
    try {
      int a = reader.read<char>();
      ASSERT_EQ(a, 16);
    } catch (StringBufferReader::BadStream& e) {
      FAIL() << "Unexpected exception. Expected success read of 1 char string";
    }
    try {
      int b = reader.read<char>();
      FAIL() << "Expected exception. Unexpected success read of empty tail";
    } catch (StringBufferReader::BadStream& e) {
    }
  }
  // Check for 2 bytes line
  {
    StringBufferReader reader(word_line, 2);
    int a, b;
    try {
      a = reader.read<char>();
      ASSERT_EQ(a, 11);
    } catch (StringBufferReader::BadStream& e) {
      FAIL() << "Unexpected exception. Expected success read of 1 char";
    }
    try {
      b = reader.fetch<char>();
      ASSERT_EQ(b, 122);
    } catch (StringBufferReader::BadStream& e) {
      FAIL() << "Unexpected exception. Expected success read of 1 char";
    }
    try {
      b = reader.fetch<uint16_t>();
      FAIL() << "Expected exception. Unexpected success read of 2 char";
    } catch (StringBufferReader::BadStream& e) {
    }
  }
}

TEST(StringBufferReader, Sequence1) {
  using namespace utils;

  char input_line[] = "\x01 \x20\x03 \x07\x80\x09\xa0";

  StringBufferReader reader(input_line, sizeof(input_line) - 1);
  uint32_t a, b, c;
  a = reader.fetch<char>();
  b = reader.fetch<uint16_t>(2);
  c = reader.fetch<uint32_t>(5);

  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 800);
  ASSERT_EQ(c, 2684977159);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}