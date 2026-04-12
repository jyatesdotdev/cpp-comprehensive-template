# Extending the Template

How to add new modules, examples, tests, dependencies, and presets to this project.

> See also: [ARCHITECTURE.md](ARCHITECTURE.md) · [TOOLCHAIN.md](TOOLCHAIN.md) · [TUTORIAL.md](TUTORIAL.md)

---

## Adding a New Library

Libraries live under `include/<module>/` (headers) and optionally `src/<module>/` (sources).

### Header-Only Library

1. Create headers in `include/mymodule/`:

```
include/mymodule/
└── widget.h
```

2. Register the CMake target in the root `CMakeLists.txt`:

```cmake
# ── MyModule Library (header-only) ───────────────────────────────────
add_library(mymodule INTERFACE)
target_include_directories(mymodule INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(mymodule INTERFACE project_warnings project_sanitizers)
```

Link additional dependencies as needed (e.g. `Threads::Threads`, `fmt::fmt`).

### Compiled Library

1. Create headers in `include/mymodule/` and sources in `src/mymodule/`:

```
include/mymodule/
└── widget.h
src/mymodule/
└── widget.cpp
```

2. Register as a `STATIC` library:

```cmake
add_library(mymodule STATIC
    src/mymodule/widget.cpp
)
target_include_directories(mymodule PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(mymodule PUBLIC project_warnings project_sanitizers)
```

### Conditional Library (Optional Dependency)

If the library requires an optional package, guard it:

```cmake
if(MyDep_FOUND)
    add_library(mymodule INTERFACE)
    target_include_directories(mymodule INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(mymodule INTERFACE MyDep::MyDep project_warnings project_sanitizers)
endif()
```

See `api` (requires `httplib`) and `database` (requires `SQLite3`) for real examples.

---

## Adding a New Example

Examples live in `examples/` and are controlled by `BUILD_EXAMPLES`.

1. Create `examples/mymodule_demo.cpp`:

```cpp
#include <mymodule/widget.h>
#include <iostream>

int main() {
    // demonstrate the module
    return 0;
}
```

2. Add to `examples/CMakeLists.txt`:

```cmake
add_executable(mymodule_demo mymodule_demo.cpp)
target_link_libraries(mymodule_demo PRIVATE mymodule)
```

If the module is conditional, guard the example:

```cmake
if(MyDep_FOUND)
    add_executable(mymodule_demo mymodule_demo.cpp)
    target_link_libraries(mymodule_demo PRIVATE mymodule)
endif()
```

---

## Adding Tests

Tests use [Catch2](https://github.com/catchorg/Catch2) and the `add_catch_test` helper in `tests/CMakeLists.txt`.

1. Create `tests/mymodule_tests.cpp`:

```cpp
#include <catch2/catch_test_macros.hpp>
#include <mymodule/widget.h>

TEST_CASE("Widget construction", "[mymodule]") {
    // ...
    REQUIRE(/* condition */);
}
```

2. Add one line to `tests/CMakeLists.txt`:

```cmake
add_catch_test(mymodule_tests mymodule)
```

The `add_catch_test(name ...)` function creates an executable from `<name>.cpp`, links `Catch2::Catch2WithMain` plus any extra libraries you pass, and registers tests via `catch_discover_tests`.

Run tests:

```bash
cmake --build build/default
ctest --test-dir build/default --output-on-failure
```

---

## Adding a Benchmark

Benchmarks use [Google Benchmark](https://github.com/google/benchmark) and live in `benchmarks/`.

1. Add benchmark cases to `benchmarks/benchmarks.cpp` (or create a new `.cpp` file):

```cpp
#include <benchmark/benchmark.h>
#include <mymodule/widget.h>

static void BM_WidgetCreate(benchmark::State& state) {
    for (auto _ : state) {
        Widget w;
        benchmark::DoNotOptimize(w);
    }
}
BENCHMARK(BM_WidgetCreate);
```

2. If using a new file, add it to `benchmarks/CMakeLists.txt`:

```cmake
target_sources(benchmarks PRIVATE my_benchmarks.cpp)
target_link_libraries(benchmarks PRIVATE mymodule)
```

Build with `BUILD_BENCHMARKS=ON`:

```bash
cmake --preset release -DBUILD_BENCHMARKS=ON
cmake --build build/release --target benchmarks
./build/release/benchmarks/benchmarks
```

---

## Adding a vcpkg Dependency

1. Add the port name to `vcpkg.json`:

```json
{
  "dependencies": [
    "existing-dep",
    "new-dep"
  ]
}
```

For optional features, add under `"features"`:

```json
{
  "features": {
    "myfeature": {
      "description": "My optional feature",
      "dependencies": ["new-dep"]
    }
  }
}
```

2. Add `find_package()` in the root `CMakeLists.txt`:

```cmake
find_package(NewDep CONFIG QUIET)
```

3. Link to your target:

```cmake
if(NewDep_FOUND)
    target_link_libraries(mymodule PUBLIC NewDep::NewDep)
    target_compile_definitions(mymodule PUBLIC HAS_NEWDEP)
endif()
```

The `QUIET` + conditional pattern keeps the build working when the dependency is absent.

---

## Adding a FetchContent Dependency

For header-only or small libraries not in vcpkg:

```cmake
include(FetchContent)
FetchContent_Declare(
    MyLib
    GIT_REPOSITORY https://github.com/org/mylib.git
    GIT_TAG        v1.0.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(MyLib)
```

Then link: `target_link_libraries(mymodule INTERFACE MyLib::MyLib)`.

See the existing `CLI11` fetch for a working example.

---

## Adding a New CMake Preset

Presets are defined in `CMakePresets.json`. There are three preset types: configure, build, and test.

### Configure Preset

```json
{
  "name": "my-preset",
  "displayName": "My Custom Preset",
  "description": "What this preset does",
  "inherits": "debug",
  "cacheVariables": {
    "MY_OPTION": "ON"
  }
}
```

Add to the `"configurePresets"` array. Use `"inherits"` to extend an existing preset (e.g. `"default"`, `"debug"`, `"release"`).

### Build Preset

```json
{
  "name": "my-preset",
  "configurePreset": "my-preset"
}
```

### Test Preset

```json
{
  "name": "my-preset",
  "configurePreset": "my-preset",
  "output": { "outputOnFailure": true },
  "environment": {
    "MY_ENV_VAR": "value"
  }
}
```

### Existing Presets Reference

| Preset | Base | Purpose |
|--------|------|---------|
| `default` | — | Standard vcpkg build |
| `debug` | `default` | Debug build |
| `release` | `default` | Release build |
| `ci` | `default` | CI: Release, tests on, examples off |
| `asan` | `debug` | AddressSanitizer |
| `ubsan` | `debug` | UndefinedBehaviorSanitizer |
| `tsan` | `debug` | ThreadSanitizer |
| `msan` | `debug` | MemorySanitizer (Clang only) |
| `asan-ubsan` | `debug` | ASan + UBSan combined |
| `security-scan` | `debug` | clang-tidy + cppcheck |

---

## Adding a CMake Option

1. Declare in root `CMakeLists.txt`:

```cmake
option(ENABLE_MY_FEATURE "Description" OFF)
```

2. Use conditionally:

```cmake
if(ENABLE_MY_FEATURE)
    # ...
endif()
```

3. Optionally expose via a preset (see above).

---

## Checklist for New Modules

- [ ] Headers in `include/<module>/` with Doxygen comments
- [ ] Sources in `src/<module>/` (if compiled)
- [ ] CMake target in root `CMakeLists.txt` linking `project_warnings` and `project_sanitizers`
- [ ] Example in `examples/` + registered in `examples/CMakeLists.txt`
- [ ] Tests in `tests/` + registered with `add_catch_test` in `tests/CMakeLists.txt`
- [ ] Dependencies added to `vcpkg.json` (if external)
- [ ] Documentation in `docs/` cross-linked from `README.md`
