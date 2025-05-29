cmake_minimum_required(VERSION 3.31.6)
project(diplom)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TEST "GTest turned on")

set(SOURCES
  src/defines.cpp
  src/utils/string_buffer_reader.cpp
  src/utils/common.cpp
  src/utils/stream_reader.cpp
  src/cdc/cdc.cpp
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

  find_package(GTest)
  target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

  add_test(AllTestsInMain main)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include)

find_package(fmt)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

find_package(otterbrix)
target_link_libraries(${PROJECT_NAME} PRIVATE otterbrix::otterbrix)

find_package(actor-zeta)
target_link_libraries(${PROJECT_NAME} PRIVATE actor-zeta::actor-zeta)

find_package(absl)
target_link_libraries(${PROJECT_NAME} PRIVATE abseil::abseil)

find_package(spdlog)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

find_package(magic_enum)
target_link_libraries(${PROJECT_NAME} PRIVATE magic_enum::magic_enum)

find_package(msgpack)
target_link_libraries(${PROJECT_NAME} PRIVATE msgpackc-cxx)

find_package(libmysqlclient)
target_link_libraries(${PROJECT_NAME} PRIVATE libmysqlclient::libmysqlclient)

