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
option(ENABLE_WERROR "Treat compiler warnings as errors" OFF)

# ── Coverage ─────────────────────────────────────────────────────────
if(ENABLE_COVERAGE)
  if(MSVC)
    message(FATAL_ERROR "ENABLE_COVERAGE requires GCC or Clang")
  endif()
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
if(ENABLE_WERROR)
    target_compile_options(project_warnings INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
endif()

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
    if(MSVC)
        message(FATAL_ERROR "Sanitizer presets in this template support GCC/Clang only")
    endif()
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
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.tar.gz
    URL_HASH SHA256=f2d893a65c3b1324c50d4e682c0cdc021dd0477ae2c048544f39eed6654b699a
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(CLI11)

# ── cpp-httplib (vcpkg/system first; header-only FetchContent fallback) ─
if(NOT httplib_FOUND)
    message(STATUS "cpp-httplib not found via find_package; fetching v0.18.3")
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        cpp_httplib
        URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.tar.gz
        URL_HASH SHA256=a0567bcd6c3fe5cef1b329b96245119047f876b49e06cc129a36a7a8dffe173e
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(cpp_httplib)
    if(TARGET httplib::httplib)
        set(httplib_FOUND TRUE)
    endif()
endif()

# ── nlohmann/json (vcpkg/system first; header-only FetchContent fallback) ─
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found via find_package; fetching v3.11.3")
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    set(JSON_CI OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
    if(TARGET nlohmann_json::nlohmann_json)
        set(nlohmann_json_FOUND TRUE)
    endif()
endif()

# ── Core Library ─────────────────────────────────────────────────────
add_library(core STATIC
    src/core/app.cpp
)
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# Tooling flags are BUILD_INTERFACE-only so they are not part of the install export.
function(cpp_link_tooling target visibility)
    target_link_libraries(${target} ${visibility}
        $<BUILD_INTERFACE:project_warnings>
        $<BUILD_INTERFACE:project_sanitizers>
    )
endfunction()

# Link an optional dep. IMPORTED packages (vcpkg/system) may appear in the export
# set; FetchContent targets stay BUILD_INTERFACE-only.
function(cpp_link_optional target visibility dep)
    if(NOT TARGET ${dep})
        return()
    endif()
    get_target_property(_imported ${dep} IMPORTED)
    if(_imported)
        target_link_libraries(${target} ${visibility} ${dep})
    else()
        target_link_libraries(${target} ${visibility} $<BUILD_INTERFACE:${dep}>)
    endif()
endfunction()

target_link_libraries(core PUBLIC Threads::Threads)
cpp_link_tooling(core PUBLIC)
if(fmt_FOUND)
    cpp_link_optional(core PUBLIC fmt::fmt)
    target_compile_definitions(core PUBLIC HAS_FMT)
endif()
if(spdlog_FOUND)
    cpp_link_optional(core PUBLIC spdlog::spdlog)
    target_compile_definitions(core PUBLIC HAS_SPDLOG)
endif()
if(nlohmann_json_FOUND)
    cpp_link_optional(core PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(core PUBLIC HAS_JSON)
endif()

# ── Memory Library ───────────────────────────────────────────────────
add_library(memory STATIC
    src/memory/resource_handle.cpp
)
target_include_directories(memory PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
cpp_link_tooling(memory PUBLIC)

# ── Concurrency Library (header-only) ────────────────────────────────
add_library(concurrency INTERFACE)
target_include_directories(concurrency INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(concurrency INTERFACE Threads::Threads)
cpp_link_tooling(concurrency INTERFACE)

# ── ETL Library (header-only) ────────────────────────────────────────
add_library(etl INTERFACE)
target_include_directories(etl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(etl INTERFACE Threads::Threads)
cpp_link_tooling(etl INTERFACE)

# ── HPC Library (header-only) ────────────────────────────────────────
add_library(hpc INTERFACE)
target_include_directories(hpc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(hpc INTERFACE Threads::Threads)
cpp_link_tooling(hpc INTERFACE)

# ── API Library (header-only, requires cpp-httplib) ──────────────────
if(httplib_FOUND)
    add_library(api INTERFACE)
    target_include_directories(api INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(api INTERFACE Threads::Threads)
    cpp_link_tooling(api INTERFACE)
    cpp_link_optional(api INTERFACE httplib::httplib)
    if(nlohmann_json_FOUND)
        cpp_link_optional(api INTERFACE nlohmann_json::nlohmann_json)
        target_compile_definitions(api INTERFACE HAS_JSON)
    endif()
endif()

# ── Patterns Library (header-only) ───────────────────────────────────
add_library(patterns INTERFACE)
target_include_directories(patterns INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
cpp_link_tooling(patterns INTERFACE)

# ── Database Library (header-only, requires SQLite3) ─────────────────
if(SQLite3_FOUND)
    if(TARGET SQLite3::SQLite3)
        set(_sqlite_target SQLite3::SQLite3)
    else()
        set(_sqlite_target SQLite::SQLite3)
    endif()
    add_library(database INTERFACE)
    target_include_directories(database INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(database INTERFACE ${_sqlite_target})
    cpp_link_tooling(database INTERFACE)
endif()

# ── Simulation Library (header-only) ─────────────────────────────────
add_library(simulation INTERFACE)
target_include_directories(simulation INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
cpp_link_tooling(simulation INTERFACE)

# ── CLI Library (header-only, uses CLI11) ────────────────────────────
add_library(cli INTERFACE)
target_include_directories(cli INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# CLI11 is FetchContent — never part of the install export.
target_link_libraries(cli INTERFACE $<BUILD_INTERFACE:CLI11::CLI11>)
cpp_link_tooling(cli INTERFACE)
if(Boost_FOUND)
    cpp_link_optional(cli INTERFACE Boost::program_options)
    target_compile_definitions(cli INTERFACE HAS_BOOST)
endif()

# ── Rendering Library (header-only, requires OpenGL + GLFW) ──────────
if(ENABLE_RENDERING AND OpenGL_FOUND)
    add_library(rendering INTERFACE)
    target_include_directories(rendering INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(rendering INTERFACE OpenGL::GL)
    cpp_link_tooling(rendering INTERFACE)
    target_compile_definitions(rendering INTERFACE ENABLE_GL)
    if(glfw3_FOUND)
        cpp_link_optional(rendering INTERFACE glfw)
    endif()
    if(glm_FOUND)
        cpp_link_optional(rendering INTERFACE glm::glm)
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

# ── Install headers + export CMake package ───────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Always-exported modules. `cli` is omitted: it INTERFACE-links FetchContent CLI11.
set(_export_targets core memory concurrency etl hpc patterns simulation)
if(TARGET api)
    list(APPEND _export_targets api)
endif()
if(TARGET database)
    list(APPEND _export_targets database)
endif()
if(TARGET rendering)
    list(APPEND _export_targets rendering)
endif()

install(TARGETS ${_export_targets} EXPORT CppComprehensiveTemplateTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

function(cpp_export_if_imported dep_target out_var)
    set(_ok FALSE)
    if(TARGET ${dep_target})
        get_target_property(_imp ${dep_target} IMPORTED)
        if(_imp)
            set(_ok TRUE)
        endif()
    endif()
    set(${out_var} ${_ok} PARENT_SCOPE)
endfunction()
cpp_export_if_imported(fmt::fmt CPP_EXPORT_FMT)
cpp_export_if_imported(spdlog::spdlog CPP_EXPORT_SPDLOG)
cpp_export_if_imported(nlohmann_json::nlohmann_json CPP_EXPORT_JSON)
cpp_export_if_imported(httplib::httplib CPP_EXPORT_HTTPLIB)
set(CPP_EXPORT_SQLITE FALSE)
if(TARGET database)
    set(CPP_EXPORT_SQLITE TRUE)
endif()
set(CPP_EXPORT_RENDERING FALSE)
if(TARGET rendering)
    set(CPP_EXPORT_RENDERING TRUE)
endif()

set(_cmake_install_dir ${CMAKE_INSTALL_LIBDIR}/cmake/CppComprehensiveTemplate)

configure_package_config_file(
    cmake/CppComprehensiveTemplateConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CppComprehensiveTemplateConfig.cmake
    INSTALL_DESTINATION ${_cmake_install_dir}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CppComprehensiveTemplateConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CppComprehensiveTemplateConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CppComprehensiveTemplateConfigVersion.cmake
    DESTINATION ${_cmake_install_dir}
)
install(EXPORT CppComprehensiveTemplateTargets
    FILE CppComprehensiveTemplateTargets.cmake
    NAMESPACE CppComprehensiveTemplate::
    DESTINATION ${_cmake_install_dir}
)

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
