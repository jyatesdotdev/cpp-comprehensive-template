if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, skipping tests")
    return()
endif()

include(Catch)

# ── Helper: add a Catch2 test executable ─────────────────────────────
function(add_catch_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARGN})
    catch_discover_tests(${name})
endfunction()

# ── Test targets ─────────────────────────────────────────────────────
add_catch_test(core_tests        core)
add_catch_test(memory_tests      memory)
add_catch_test(concurrency_tests concurrency)
add_catch_test(etl_tests         etl)
add_catch_test(patterns_tests    patterns)
add_catch_test(simulation_tests  simulation)
add_catch_test(cli_tests         cli)
