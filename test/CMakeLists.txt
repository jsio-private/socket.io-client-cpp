cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt)

add_executable(sio_test sio_test.cpp)

set_property(TARGET sio_test PROPERTY CXX_STANDARD 11)
set_property(TARGET sio_test PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(sioclient PRIVATE ${Boost_LIBRARIES})
target_link_libraries(sio_test sioclient)


set(GTEST ${CMAKE_CURRENT_SOURCE_DIR}/lib/googletest)
enable_testing()
add_subdirectory(${GTEST})
include_directories(
  ${GTEST}/googletest/include
  ${GTEST}/googletest
  ${GTEST}/googlemock/include
)
target_link_libraries(sio_test gtest gtest_main)
add_test(NAME test COMMAND sio_test)
add_custom_target(
  check ${CMAKE_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1
  ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --verbose
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


target_include_directories(sio_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
