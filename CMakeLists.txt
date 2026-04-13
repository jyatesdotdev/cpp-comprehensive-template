cmake_minimum_required(VERSION 3.21)

project(CppComprehensiveTemplate
    VERSION 1.0.0
    DESCRIPTION "Modern C++ comprehensive project template"
    LANGUAGES CXX
)

# ── Modern C++ Standard ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ──────────────────────────────────────────────────────────
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks (Google Benchmark)" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer (Clang only)" OFF)
option(ENABLE_RENDERING "Build rendering module (requires OpenGL)" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

# ── Coverage ─────────────────────────────────────────────────────────
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
  add_link_options(--coverage)
endif()

# ── Security Tooling ────────────────────────────────────────────────
include(cmake/Security.cmake)

# ── Compiler Warnings ───────────────────────────────────────────────
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
        -Wold-style-cast -Wcast-align -Woverloaded-virtual
        -Wconversion -Wsign-conversion -Wnull-dereference
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /permissive-
    >
)

# ── Sanitizers ───────────────────────────────────────────────────────
# Mutual exclusion checks
if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be used together")
endif()
if(ENABLE_ASAN AND ENABLE_MSAN)
    message(FATAL_ERROR "ASan and MSan cannot be used together")
endif()
if(ENABLE_TSAN AND ENABLE_MSAN)
    message(FATAL_ERROR "TSan and MSan cannot be used together")
endif()
if(ENABLE_MSAN AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "MSan requires Clang")
endif()

add_library(project_sanitizers INTERFACE)

set(_SANITIZER_FLAGS "")
if(ENABLE_ASAN)
    list(APPEND _SANITIZER_FLAGS "address")
endif()
if(ENABLE_UBSAN)
    list(APPEND _SANITIZER_FLAGS "undefined")
endif()
if(ENABLE_TSAN)
    list(APPEND _SANITIZER_FLAGS "thread")
endif()
if(ENABLE_MSAN)
    list(APPEND _SANITIZER_FLAGS "memory")
endif()

if(_SANITIZER_FLAGS)
    list(JOIN _SANITIZER_FLAGS "," _SANITIZER_LIST)
    target_compile_options(project_sanitizers INTERFACE
        -fsanitize=${_SANITIZER_LIST} -fno-omit-frame-pointer
    )
    target_link_options(project_sanitizers INTERFACE
        -fsanitize=${_SANITIZER_LIST}
    )
    message(STATUS "Sanitizers enabled: ${_SANITIZER_LIST}")
endif()

# ── Find Dependencies via vcpkg / system ─────────────────────────────
find_package(fmt CONFIG QUIET)
find_package(spdlog CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)
find_package(Catch2 3 CONFIG QUIET)
find_package(benchmark CONFIG QUIET)
find_package(httplib CONFIG QUIET)
find_package(SQLite3 QUIET)
find_package(Threads REQUIRED)

if(ENABLE_RENDERING)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 CONFIG QUIET)
    find_package(glm CONFIG QUIET)
endif()

find_package(Boost CONFIG QUIET COMPONENTS program_options)

# ── CLI11 (header-only, via FetchContent) ────────────────────────────
include(FetchContent)
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(CLI11)

# ── Core Library ─────────────────────────────────────────────────────
add_library(core STATIC
    src/core/app.cpp
)
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(core PUBLIC
    Threads::Threads
    project_warnings
    project_sanitizers
)
if(fmt_FOUND)
    target_link_libraries(core PUBLIC fmt::fmt)
    target_compile_definitions(core PUBLIC HAS_FMT)
endif()
if(spdlog_FOUND)
    target_link_libraries(core PUBLIC spdlog::spdlog)
    target_compile_definitions(core PUBLIC HAS_SPDLOG)
endif()
if(nlohmann_json_FOUND)
    target_link_libraries(core PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(core PUBLIC HAS_JSON)
endif()
if(Boost_FOUND)
    target_link_libraries(core PUBLIC Boost::program_options)
    target_compile_definitions(core PUBLIC HAS_BOOST)
endif()

# ── Memory Library ───────────────────────────────────────────────────
add_library(memory STATIC
    src/memory/resource_handle.cpp
)
target_include_directories(memory PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(memory PUBLIC project_warnings project_sanitizers)

# ── Concurrency Library (header-only) ────────────────────────────────
add_library(concurrency INTERFACE)
target_include_directories(concurrency INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(concurrency INTERFACE Threads::Threads project_warnings project_sanitizers)

# ── ETL Library (header-only) ────────────────────────────────────────
add_library(etl INTERFACE)
target_include_directories(etl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(etl INTERFACE Threads::Threads project_warnings project_sanitizers)

# ── HPC Library (header-only) ────────────────────────────────────────
add_library(hpc INTERFACE)
target_include_directories(hpc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(hpc INTERFACE Threads::Threads project_warnings project_sanitizers)

# ── API Library (header-only, requires cpp-httplib) ──────────────────
if(httplib_FOUND)
    add_library(api INTERFACE)
    target_include_directories(api INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(api INTERFACE
        httplib::httplib Threads::Threads project_warnings project_sanitizers
    )
    if(nlohmann_json_FOUND)
        target_link_libraries(api INTERFACE nlohmann_json::nlohmann_json)
        target_compile_definitions(api INTERFACE HAS_JSON)
    endif()
endif()

# ── Patterns Library (header-only) ───────────────────────────────────
add_library(patterns INTERFACE)
target_include_directories(patterns INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(patterns INTERFACE project_warnings project_sanitizers)

# ── Database Library (header-only, requires SQLite3) ─────────────────
if(SQLite3_FOUND)
    add_library(database INTERFACE)
    target_include_directories(database INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(database INTERFACE
        SQLite::SQLite3 project_warnings project_sanitizers
    )
endif()

# ── Simulation Library (header-only) ─────────────────────────────────
add_library(simulation INTERFACE)
target_include_directories(simulation INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(simulation INTERFACE project_warnings project_sanitizers)

# ── CLI Library (header-only, uses CLI11) ────────────────────────────
add_library(cli INTERFACE)
target_include_directories(cli INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cli INTERFACE CLI11::CLI11 project_warnings project_sanitizers)
if(Boost_FOUND)
    target_link_libraries(cli INTERFACE Boost::program_options)
    target_compile_definitions(cli INTERFACE HAS_BOOST)
endif()

# ── Rendering Library (header-only, requires OpenGL + GLFW) ──────────
if(ENABLE_RENDERING AND OpenGL_FOUND)
    add_library(rendering INTERFACE)
    target_include_directories(rendering INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(rendering INTERFACE
        OpenGL::GL project_warnings project_sanitizers
    )
    target_compile_definitions(rendering INTERFACE ENABLE_GL)
    if(glfw3_FOUND)
        target_link_libraries(rendering INTERFACE glfw)
    endif()
    if(glm_FOUND)
        target_link_libraries(rendering INTERFACE glm::glm)
    endif()
endif()

# ── Subdirectories ───────────────────────────────────────────────────
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS AND Catch2_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS AND benchmark_FOUND)
    add_subdirectory(benchmarks)
endif()

# ── Documentation (Doxygen) ──────────────────────────────────────────
if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    doxygen_add_docs(docs
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/docs
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif()
