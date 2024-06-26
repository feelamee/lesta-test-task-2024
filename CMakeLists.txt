cmake_minimum_required(VERSION 3.5)
project(
    tt
    LANGUAGES CXX
    DESCRIPTION "test task for Lesta Games")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

add_library(pedantic INTERFACE)
target_compile_options(
    pedantic
    INTERFACE -Wall
              -pedantic
              -Werror)

add_library(asan INTERFACE)
target_compile_options(asan INTERFACE -fsanitize=address)
target_link_options(
    asan
    INTERFACE
    -fsanitize=address)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/bench)

add_library(tt INTERFACE)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/tt)
target_sources(
    tt
    PRIVATE ${SOURCE_DIR}/iseven.hpp
            ${SOURCE_DIR}/lock_free_ringbuf.hpp
            ${SOURCE_DIR}/ringbuf.hpp
            ${SOURCE_DIR}/sort.hpp)
target_include_directories(tt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_compile_features(tt INTERFACE cxx_std_23)
