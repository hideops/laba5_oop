cmake_minimum_required(VERSION 3.10)
project(PmrQueue)

set(CMAKE_CXX_STANDARD 20)

enable_testing()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(main main.cpp)
add_executable(tests test_pmr_queue.cpp)
target_link_libraries(tests ${GTEST_LIBRARIES} pthread)

add_test(NAME PmrQueueTest COMMAND tests)
