add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world PRIVATE core)

add_executable(memory_demo memory_demo.cpp)
target_link_libraries(memory_demo PRIVATE memory)

add_executable(concurrency_demo concurrency_demo.cpp)
target_link_libraries(concurrency_demo PRIVATE concurrency)

add_executable(hpc_demo hpc_demo.cpp)
target_link_libraries(hpc_demo PRIVATE hpc)

add_executable(etl_demo etl_demo.cpp)
target_link_libraries(etl_demo PRIVATE etl)

add_executable(patterns_demo patterns_demo.cpp)
target_link_libraries(patterns_demo PRIVATE patterns)

add_executable(simulation_demo simulation_demo.cpp)
target_link_libraries(simulation_demo PRIVATE simulation)

if(httplib_FOUND)
    add_executable(api_demo api_demo.cpp)
    target_link_libraries(api_demo PRIVATE api)
endif()

if(SQLite3_FOUND)
    add_executable(database_demo database_demo.cpp)
    target_link_libraries(database_demo PRIVATE database)
endif()

if(ENABLE_RENDERING AND OpenGL_FOUND AND glfw3_FOUND)
    add_executable(rendering_demo rendering_demo.cpp)
    target_link_libraries(rendering_demo PRIVATE rendering)
endif()

# CLI demo (CLI11 subcommands, validators, callbacks)
add_executable(cli_demo cli_demo.cpp)
target_link_libraries(cli_demo PRIVATE cli)

# Third-party integration demo (uses core which links fmt/spdlog/json when found)
add_executable(third_party_demo third_party_demo.cpp)
target_link_libraries(third_party_demo PRIVATE core)
