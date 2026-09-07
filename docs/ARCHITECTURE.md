# Architecture

This document describes the project structure, module hierarchy, dependency graph, CMake target relationships, and design patterns used in the C++ Comprehensive Template.

## Project Layout

```
├── CMakeLists.txt            # Root build — defines all library targets
├── CMakePresets.json          # Configure/build/test presets
├── vcpkg.json                 # vcpkg dependency manifest
├── Doxyfile                   # Doxygen configuration
├── .clang-tidy                # Static analysis rules
├── .clang-format              # Code formatting rules
├── cmake/
│   ├── Security.cmake         # clang-tidy, cppcheck, IWYU, Valgrind integration
│   └── CppComprehensiveTemplateConfig.cmake.in  # install() package config
├── include/                   # Public headers (one subdirectory per module)
│   ├── core/                  # App lifecycle, RAII, pimpl
│   ├── memory/                # Smart pointers, allocators, RAII handles
│   ├── concurrency/           # Thread pool, lock-free queue, parallel utilities
│   ├── hpc/                   # SIMD intrinsics, parallel STL
│   ├── etl/                   # Data pipelines, MapReduce
│   ├── api/                   # REST client/server (cpp-httplib)
│   ├── database/              # SQLite wrapper, repository pattern
│   ├── patterns/              # CRTP, type erasure, visitor, observer
│   ├── rendering/             # OpenGL pipeline, shaders
│   ├── simulation/            # Physics, ECS, numerical integration
│   └── cli/                   # CLI11 helpers, output formatting
├── src/                       # Compiled modules only (`core`, `memory`)
│   ├── core/app.cpp           # App pimpl implementation
│   └── memory/resource_handle.cpp
├── examples/                  # Runnable demo programs (one per module)
├── tests/                     # Catch2 unit tests (one per module; rendering is example-only)
├── benchmarks/                # Google Benchmark targets
├── docs/                      # Documentation and guides
├── LICENSE                    # MIT
└── .github/workflows/         # CI (build/test/coverage) + security scanning
```

## Module / Namespace Hierarchy

Each module maps to a namespace, a CMake target, a header directory, and (optionally) a source directory:

| Module | Namespace | CMake Target | Type | Key Headers |
|---|---|---|---|---|
| core | `core` | `core` | STATIC | `app.h` |
| memory | `memory` | `memory` | STATIC | `arena_allocator.h`, `resource_handle.h`, `smart_pointers.h` |
| concurrency | `concurrency` | `concurrency` | INTERFACE | `thread_pool.h`, `lock_free_queue.h`, `parallel.h` |
| hpc | `hpc` | `hpc` | INTERFACE | `simd_ops.h`, `parallel_stl.h` |
| etl | `etl` | `etl` | INTERFACE | `pipeline.h`, `map_reduce.h` |
| api | `api` | `api` | INTERFACE | `rest_server.h`, `rest_client.h` |
| database | `database` | `database` | INTERFACE | `sqlite.h` |
| patterns | `patterns` | `patterns` | INTERFACE | `crtp.h`, `type_erasure.h`, `visitor.h`, `observer.h` |
| rendering | `rendering` | `rendering` | INTERFACE | `render_pipeline.h`, `shader.h`, `gl_resource.h` |
| simulation | `simulation` | `simulation` | INTERFACE | `physics.h`, `ecs.h`, `numerical.h` |
| cli | `cli` | `cli` | INTERFACE | `cli_helpers.h`, `output_format.h` |

Most modules are header-only (`INTERFACE`). Only `core` and `memory` have `.cpp` files and produce static libraries.

## Dependency Graph

### CMake Target Dependencies

```
project_warnings (INTERFACE)   ─┐
project_sanitizers (INTERFACE)  ─┤── linked by ALL library targets
Threads::Threads               ─┘

core ──────► fmt::fmt (optional, HAS_FMT)
           ► spdlog::spdlog (optional, HAS_SPDLOG)
           ► nlohmann_json::nlohmann_json (optional, HAS_JSON)

memory ────► (no external deps)

concurrency ► Threads::Threads

hpc ────────► Threads::Threads

etl ────────► Threads::Threads

api ────────► httplib::httplib (vcpkg/system, else FetchContent v0.18.3)
           ► nlohmann_json::nlohmann_json (optional, HAS_JSON)
           ► Threads::Threads

database ──► SQLite3::SQLite3 (or SQLite::SQLite3; target only created if found)

patterns ──► (no external deps)

rendering ─► OpenGL::GL (required — target only created if ENABLE_RENDERING)
           ► glfw (optional)
           ► glm::glm (optional)

simulation ► (no external deps)

cli ───────► CLI11::CLI11 (FetchContent)
           ► Boost::program_options (optional, HAS_BOOST)
```

### Conditional Targets

Some targets are only defined when their dependencies are found:

- `api` — requires `httplib_FOUND` (package or FetchContent fallback)
- `database` — requires `SQLite3_FOUND`
- `rendering` — requires `ENABLE_RENDERING=ON` and `OpenGL_FOUND`

### External Dependencies

| Source | Libraries |
|---|---|
| vcpkg (required) | fmt, spdlog, nlohmann-json, catch2, benchmark, cpp-httplib, sqlite3 |
| vcpkg (feature: rendering) | glfw3, glm, glad |
| vcpkg (feature: boost) | boost-asio, boost-beast, boost-program-options |
| FetchContent | CLI11 v2.4.2; cpp-httplib v0.18.3 and nlohmann/json v3.11.3 (if the matching `find_package` fails) |

## CMake Build System

### INTERFACE Targets

- `project_warnings` — compiler warning flags (GCC/Clang/MSVC), linked by all libraries
- `project_sanitizers` — sanitizer flags (ASan, UBSan, TSan, MSan), linked by all libraries

### Build Options

| Option | Default | Description |
|---|---|---|
| `BUILD_EXAMPLES` | ON | Build example programs |
| `BUILD_TESTS` | ON | Build Catch2 tests |
| `BUILD_BENCHMARKS` | OFF | Build Google Benchmark targets |
| `BUILD_DOCS` | OFF | Build Doxygen documentation |
| `ENABLE_ASAN` | OFF | AddressSanitizer |
| `ENABLE_UBSAN` | OFF | UndefinedBehaviorSanitizer |
| `ENABLE_TSAN` | OFF | ThreadSanitizer |
| `ENABLE_MSAN` | OFF | MemorySanitizer (Clang only) |
| `ENABLE_RENDERING` | OFF | OpenGL rendering module |
| `ENABLE_CLANG_TIDY` | OFF | clang-tidy static analysis |
| `ENABLE_CPPCHECK` | OFF | cppcheck static analysis |
| `ENABLE_IWYU` | OFF | include-what-you-use |
| `ENABLE_VALGRIND` | OFF | Valgrind memcheck target |
| `ENABLE_COVERAGE` | OFF | gcov/lcov instrumentation |
| `ENABLE_WERROR` | OFF | Treat compiler warnings as errors (ON in CI presets) |

`cmake --install` exports `CppComprehensiveTemplate::` targets (not `cli`) and
`CppComprehensiveTemplateConfig.cmake`. FetchContent deps and `project_warnings` /
`project_sanitizers` are `$<BUILD_INTERFACE:...>` only.

### Presets

| Preset | Inherits | Purpose |
|---|---|---|
| `default` | — | Standard vcpkg build (`RelWithDebInfo`) |
| `debug` | default | Debug build |
| `release` | default | Optimized release |
| `ci` | default | CI: tests on, examples off, `-Werror` |
| `asan` | debug | AddressSanitizer (`-Werror`) |
| `ubsan` | debug | UndefinedBehaviorSanitizer (`-Werror`) |
| `tsan` | debug | ThreadSanitizer (`-Werror`) |
| `msan` | debug | MemorySanitizer (`-Werror`, Clang) |
| `asan-ubsan` | debug | ASan + UBSan combined (`-Werror`) |
| `security-scan` | debug | clang-tidy + cppcheck (`-Werror`) |
| `coverage` | debug | gcov coverage, 80% CI gate (`-Werror`) |

## Design Patterns Used

| Pattern | Location | Description |
|---|---|---|
| Pimpl (pointer-to-implementation) | `core::App` | Hides implementation behind `std::unique_ptr<Impl>`, preserving ABI stability |
| RAII | `core::App`, `memory::UniqueHandle`, `rendering::gl::Resource` / `gl::Shader` / `gl::Program` | Resource acquisition in constructor, release in destructor |
| CRTP (Curiously Recurring Template Pattern) | `patterns/crtp.h` | Static polymorphism without virtual dispatch |
| Type Erasure | `patterns/type_erasure.h` | Runtime polymorphism without inheritance |
| Visitor | `patterns/visitor.h` | Double dispatch via `std::variant` and `std::visit` |
| Observer | `patterns/observer.h` | Event notification with type-safe callbacks |
| Repository | `database/sqlite.h` | Data access abstraction over SQLite |
| Pipeline / Chain | `etl/pipeline.h` | Composable data transformation stages |
| MapReduce | `etl/map_reduce.h` | Parallel map + reduce over data sets |
| Thread Pool | `concurrency/thread_pool.h` | Fixed-size pool with `std::future`-based task submission |
| Lock-Free Queue | `concurrency/lock_free_queue.h` | Atomic-based concurrent queue |
| Entity Component System | `simulation/ecs.h` | Data-oriented game/simulation architecture |
| Render Pipeline | `rendering/render_pipeline.h` | Multi-pass rendering with configurable stages |

## Related Documentation

- [README](../README.md) — Quick start and project overview
- [Tutorial](TUTORIAL.md) — New developer walkthrough
- [Toolchain](TOOLCHAIN.md) — Required tools, versions, IDE setup
- [Extending](EXTENDING.md) — Adding libraries, examples, tests, dependencies
- [C++ API guidelines](api_design.md) — C++ API design (not REST)
- [Best Practices](best_practices.md) — C++20 guidelines (C++23 called out as future)
- [CLI](cli.md) — Command-line interface design
- [Cross-Platform Build](cross_platform_build.md) — Platform-specific build notes
- [HPC Optimization](hpc_optimization.md) — SIMD and parallel performance
- [Rendering Pipeline](rendering_pipeline.md) — Graphics architecture
- [Security Scanning](SECURITY_SCANNING.md) — Static analysis and sanitizers
- [Third-Party Integration](third_party_integration.md) — Dependency management
