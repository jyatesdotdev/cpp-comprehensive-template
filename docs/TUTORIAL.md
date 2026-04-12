# New Developer Tutorial

A hands-on walkthrough from cloning the repository to adding your first feature.

> **Time estimate:** ~30 minutes  
> **Prerequisites:** See [TOOLCHAIN.md](TOOLCHAIN.md) for required tools and installation instructions.

## Table of Contents

1. [Clone & Verify Tools](#1-clone--verify-tools)
2. [Configure with CMake Presets](#2-configure-with-cmake-presets)
3. [Build](#3-build)
4. [Run Tests](#4-run-tests)
5. [Run Examples](#5-run-examples)
6. [Add a Feature](#6-add-a-feature)
7. [Run Security Scans](#7-run-security-scans)
8. [Next Steps](#8-next-steps)

---

## 1. Clone & Verify Tools

```bash
git clone <repo-url> && cd cpp-comprehensive-template
```

Verify your toolchain:

```bash
cmake --version          # ≥ 3.21
g++ --version            # GCC 12+ (or clang++ 15+, or MSVC 2022+)
echo $VCPKG_ROOT         # Should point to your vcpkg installation
```

If `VCPKG_ROOT` is not set, install vcpkg first:

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg   # Add to your shell profile
```

## 2. Configure with CMake Presets

List available presets:

```bash
cmake --list-presets
```

| Preset | Use Case |
|--------|----------|
| `default` | Standard development build (vcpkg, Debug) |
| `debug` | Explicit debug build |
| `release` | Optimized release build |
| `ci` | CI: tests ON, examples OFF |
| `asan` | AddressSanitizer (memory errors) |
| `ubsan` | UndefinedBehaviorSanitizer |
| `tsan` | ThreadSanitizer (data races) |
| `msan` | MemorySanitizer (Clang only) |
| `asan-ubsan` | ASan + UBSan combined |
| `security-scan` | clang-tidy + cppcheck static analysis |

Configure the default preset:

```bash
cmake --preset default
```

This creates `build/default/` and uses vcpkg to fetch dependencies (fmt, spdlog, nlohmann-json, Catch2, cpp-httplib, sqlite3, etc.). The first configure may take a few minutes while vcpkg builds packages.

## 3. Build

```bash
cmake --build --preset default
```

Build artifacts go to `build/default/`. To build a specific target:

```bash
cmake --build --preset default --target hello_world
```

For a parallel build (faster on multi-core machines):

```bash
cmake --build --preset default -j$(nproc)   # Linux
cmake --build --preset default -j$(sysctl -n hw.ncpu)  # macOS
```

## 4. Run Tests

Tests use [Catch2](https://github.com/catchorg/Catch2) and are discovered automatically by CTest.

```bash
# Run all tests
ctest --preset default

# Verbose output (shows individual test cases)
ctest --preset default --output-on-failure -V
```

Available test suites:

| Test Target | Module | Source |
|-------------|--------|--------|
| `core_tests` | core | `tests/core_tests.cpp` |
| `memory_tests` | memory | `tests/memory_tests.cpp` |
| `concurrency_tests` | concurrency | `tests/concurrency_tests.cpp` |
| `etl_tests` | etl | `tests/etl_tests.cpp` |
| `patterns_tests` | patterns | `tests/patterns_tests.cpp` |
| `simulation_tests` | simulation | `tests/simulation_tests.cpp` |
| `cli_tests` | cli | `tests/cli_tests.cpp` |

Run a single test executable directly for more control:

```bash
./build/default/tests/core_tests --list-tests     # List test cases
./build/default/tests/core_tests "[tag]"           # Run by tag
```

## 5. Run Examples

Each module has a demo program in `examples/`:

```bash
# Core — RAII app lifecycle
./build/default/examples/hello_world

# Memory — smart pointers, arena allocator, RAII handles
./build/default/examples/memory_demo

# Concurrency — thread pool, async, lock-free queue
./build/default/examples/concurrency_demo

# HPC — SIMD operations, parallel STL
./build/default/examples/hpc_demo

# ETL — data pipelines, map-reduce
./build/default/examples/etl_demo

# Patterns — CRTP, visitor, observer, type erasure
./build/default/examples/patterns_demo

# Simulation — physics, ECS, numerical integration
./build/default/examples/simulation_demo

# API — REST client/server (requires cpp-httplib)
./build/default/examples/api_demo

# Database — SQLite wrapper (requires sqlite3)
./build/default/examples/database_demo

# CLI — subcommands, validators, output formatting
./build/default/examples/cli_demo --help

# Third-party integration — fmt, spdlog, nlohmann-json
./build/default/examples/third_party_demo
```

> **Note:** `api_demo`, `database_demo`, and `rendering_demo` are only built when their dependencies are found. The rendering demo also requires `-DENABLE_RENDERING=ON`.

## 6. Add a Feature

Let's walk through adding a `greet()` function to the `core` module.

### 6a. Add the header declaration

Edit `include/core/app.h` — add inside the `core` namespace:

```cpp
/// @brief Generate a greeting message.
/// @param name The name to greet.
/// @return A formatted greeting string.
[[nodiscard]] std::string greet(std::string_view name);
```

### 6b. Add the implementation

Edit `src/core/app.cpp` — add the function body:

```cpp
std::string core::greet(std::string_view name) {
    return std::string("Hello, ") + std::string(name) + "!";
}
```

### 6c. Add a test

Edit `tests/core_tests.cpp` — add a test case:

```cpp
TEST_CASE("greet returns greeting", "[core]") {
    REQUIRE(core::greet("World") == "Hello, World!");
    REQUIRE(core::greet("") == "Hello, !");
}
```

### 6d. Build and test

```bash
cmake --build --preset default --target core_tests
./build/default/tests/core_tests "[core]"
```

### 6e. Use it in an example (optional)

Edit `examples/hello_world.cpp`:

```cpp
#include "core/app.h"
#include <iostream>

int main() {
    core::App app({.name = "HelloWorld", .log_level = 1});
    std::cout << core::greet("Developer") << "\n";
    return app.run();
}
```

For more complex additions (new libraries, new modules, new dependencies), see [EXTENDING.md](EXTENDING.md).

## 7. Run Security Scans

### Static analysis (clang-tidy + cppcheck)

```bash
cmake --preset security-scan
cmake --build --preset security-scan
```

Findings appear as compiler warnings/errors during the build. Security-critical checks (CERT rules, null dereference, memory leaks) are configured to break the build.

### Runtime sanitizers

The most useful combination for development is ASan + UBSan:

```bash
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan
```

This detects memory errors (out-of-bounds, use-after-free, leaks) and undefined behavior (signed overflow, null dereference) at runtime.

For concurrency bugs, use ThreadSanitizer:

```bash
cmake --preset tsan
cmake --build --preset tsan
ctest --preset tsan
```

> **Important:** ASan and TSan cannot be combined. Use separate build directories (presets handle this automatically).

For full details on all security tools, suppression patterns, and CI integration, see [SECURITY_SCANNING.md](SECURITY_SCANNING.md).

## 8. Next Steps

- [ARCHITECTURE.md](ARCHITECTURE.md) — Project structure, module hierarchy, dependency graph
- [EXTENDING.md](EXTENDING.md) — Adding libraries, examples, tests, dependencies, presets
- [TOOLCHAIN.md](TOOLCHAIN.md) — Tool versions, platform-specific installation, IDE setup
- [best_practices.md](best_practices.md) — C++20 coding guidelines and patterns
- [api_design.md](api_design.md) — REST API design patterns
- [cli.md](cli.md) — CLI framework with CLI11
- [cross_platform_build.md](cross_platform_build.md) — Cross-platform CMake configuration
- [hpc_optimization.md](hpc_optimization.md) — SIMD and parallel algorithm optimization
- [rendering_pipeline.md](rendering_pipeline.md) — OpenGL rendering pipeline
- [third_party_integration.md](third_party_integration.md) — Integrating external libraries
- [SECURITY_SCANNING.md](SECURITY_SCANNING.md) — Static analysis, sanitizers, CI security
