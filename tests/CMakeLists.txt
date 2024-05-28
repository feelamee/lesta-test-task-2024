cmake_minimum_required(VERSION 3.5)
project(tests LANGUAGES CXX)

add_executable(tests)

set(TESTS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(
    tests
    PRIVATE ${TESTS_SOURCE_DIR}/iseven.test.cpp
            ${TESTS_SOURCE_DIR}/lock_free_ringbuf.test.cpp
            ${TESTS_SOURCE_DIR}/main.test.cpp
            ${TESTS_SOURCE_DIR}/ringbuf.test.cpp
            ${TESTS_SOURCE_DIR}/sort.test.cpp)

find_package(doctest REQUIRED)
target_link_libraries(tests PRIVATE tt doctest::doctest)

# TODO: use cmake presets instead of this...
if(TT_TESTS_PEDANTIC)
    message(STATUS "tests: enabled pedantic mode")
    target_link_libraries(tests PRIVATE pedantic)
endif()

if(TT_TESTS_ASAN)
    message(STATUS "tests: enabled address sanitizer")
    target_link_libraries(tests PRIVATE asan)
endif()
