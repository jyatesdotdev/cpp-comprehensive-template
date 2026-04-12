if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found, skipping benchmarks")
    return()
endif()

add_executable(benchmarks benchmarks.cpp)
target_link_libraries(benchmarks PRIVATE
    benchmark::benchmark benchmark::benchmark_main
    memory etl concurrency
)
