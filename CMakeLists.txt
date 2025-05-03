cmake_minimum_required(VERSION 3.31.6)
project(diplom)

include(/home/nikita/vcpkg/scripts/buildsystems/vcpkg.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TEST "GTest turned on")

set(SOURCES
  src/defines.cpp
  src/utils/string_buffer_reader.cpp
  src/utils/common.cpp
  src/utils/stream_reader.cpp
  src/binlog/binlog_events.cpp
  src/binlog/binlog_reader.cpp
)

if(BUILD_TESTS)
  set(SOURCES ${SOURCES} test/test.cpp)
else()
  set(SOURCES ${SOURCES} src/main.cpp)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

if(BUILD_TESTS)
  enable_testing()

  find_package(GTest CONFIG REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

  add_test(AllTestsInMain main)
endif()

target_include_directories(diplom PRIVATE include)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(diplom PRIVATE fmt::fmt)
