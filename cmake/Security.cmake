# cmake/Security.cmake — Centralized security tooling module
#
# Options:
#   ENABLE_CLANG_TIDY   — static analysis via clang-tidy (uses .clang-tidy config)
#   ENABLE_CPPCHECK     — static analysis for UB, buffer overflows, leaks
#   ENABLE_IWYU         — include-what-you-use header analysis
#   ENABLE_VALGRIND     — adds a memcheck CTest target
#
# Usage: include(cmake/Security.cmake) after project()

option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)
option(ENABLE_IWYU "Enable include-what-you-use" OFF)
option(ENABLE_VALGRIND "Enable Valgrind memcheck target" OFF)

# ── clang-tidy ───────────────────────────────────────────────────────
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXE}"
            "--warnings-as-errors=cert-*,clang-analyzer-security.*"
        )
    else()
        message(WARNING "ENABLE_CLANG_TIDY is ON but clang-tidy not found")
    endif()
endif()

# ── cppcheck ─────────────────────────────────────────────────────────
if(ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        message(STATUS "cppcheck found: ${CPPCHECK_EXE}")
        set(CMAKE_CXX_CPPCHECK
            "${CPPCHECK_EXE}"
            "--enable=warning,performance,portability"
            "--suppress=missingIncludeSystem"
            "--inline-suppr"
            "--error-exitcode=1"
            "--std=c++20"
        )
    else()
        message(WARNING "ENABLE_CPPCHECK is ON but cppcheck not found")
    endif()
endif()

# ── include-what-you-use ─────────────────────────────────────────────
if(ENABLE_IWYU)
    find_program(IWYU_EXE NAMES include-what-you-use iwyu)
    if(IWYU_EXE)
        message(STATUS "include-what-you-use found: ${IWYU_EXE}")
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}")
    else()
        message(WARNING "ENABLE_IWYU is ON but include-what-you-use not found")
    endif()
endif()

# ── Valgrind memcheck target ─────────────────────────────────────────
if(ENABLE_VALGRIND)
    find_program(VALGRIND_EXE NAMES valgrind)
    if(VALGRIND_EXE)
        message(STATUS "Valgrind found: ${VALGRIND_EXE}")
        set(MEMORYCHECK_COMMAND "${VALGRIND_EXE}")
        set(MEMORYCHECK_COMMAND_OPTIONS
            "--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"
        )
        # Usage: cmake --build build && cd build && ctest -T memcheck
        include(CTest)
    else()
        message(WARNING "ENABLE_VALGRIND is ON but valgrind not found")
    endif()
endif()
