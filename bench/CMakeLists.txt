cmake_minimum_required(VERSION 3.5)
project(bench-sort)

add_executable(bench-sort)
target_sources(bench-sort PRIVATE main.cpp)

find_package(benchmark REQUIRED)
target_link_libraries(bench-sort PRIVATE tt benchmark::benchmark_main)

# TODO: use cmake presets instead of this...
if(TT_BENCH_PEDANTIC)
    message(STATUS "bench: enabled pedantic mode")
    target_link_libraries(bench-sort PRIVATE pedantic)
endif()

if(TT_BENCH_ASAN)
    message(STATUS "bench: enabled address sanitizer")
    target_link_libraries(bench-sort PRIVATE asan)
endif()
