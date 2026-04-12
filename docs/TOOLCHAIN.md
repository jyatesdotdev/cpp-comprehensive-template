# Toolchain & Development Environment

Required and optional tools for building, testing, and developing with this template.

> See also: [Cross-Platform Build](cross_platform_build.md) for build commands, [SECURITY_SCANNING.md](SECURITY_SCANNING.md) for analysis tools, [ARCHITECTURE.md](ARCHITECTURE.md) for project structure.

---

## Required Tools

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| **C++20 compiler** | GCC 12+, Clang 15+, MSVC 17.4+, Apple Clang (Xcode 14.3+) | Language standard |
| **CMake** | 3.21 | Build system generator |
| **vcpkg** | latest | C++ package manager (recommended) |

### Compiler Feature Notes

| Feature | GCC | Clang | MSVC | Apple Clang |
|---------|-----|-------|------|-------------|
| Concepts | 12+ | 15+ | 17.4+ | 14.3+ |
| Ranges | 12+ | 15+ | 17.4+ | 14.3+ |
| `std::jthread` | 11+ | 17+ (libc++) | 2022+ | ❌ |
| `<execution>` (parallel STL) | 12+ (with TBB) | ❌ | 17.4+ | ❌ |
| `std::format` | 13+ | 17+ | 17.4+ | ❌ |

> `fmt` is used as a fallback when `std::format` is unavailable (controlled by `HAS_FMT` define).

---

## Installation by Platform

### macOS

```bash
# Xcode command-line tools (provides Apple Clang)
xcode-select --install

# CMake
brew install cmake

# vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.zshrc

# Optional: GCC (for parallel STL / jthread)
brew install gcc
# Then: cmake -B build -DCMAKE_CXX_COMPILER=g++-13
```

### Ubuntu / Debian

```bash
# GCC 12+
sudo apt update
sudo apt install g++-12 cmake ninja-build

# Or Clang 15+
sudo apt install clang-15 libc++-15-dev libc++abi-15-dev

# vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
```

### Fedora / RHEL

```bash
sudo dnf install gcc-c++ cmake ninja-build
# Or: sudo dnf install clang clang-tools-extra

# vcpkg (same as above)
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
```

### Windows

```powershell
# Visual Studio 2022 (17.4+) with "Desktop development with C++" workload
# Includes MSVC compiler, CMake, and Ninja

# Or install CMake separately
winget install Kitware.CMake

# vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
# Set VCPKG_ROOT=C:\vcpkg in system environment variables
```

### Windows (MSYS2 / MinGW)

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-ninja
```

---

## Optional Tools

### Static Analysis

| Tool | Install | Used By |
|------|---------|---------|
| **clang-tidy** (16/17/18) | Part of `clang-tools-extra` | `ENABLE_CLANG_TIDY`, `security-scan` preset |
| **cppcheck** | `brew install cppcheck` / `apt install cppcheck` | `ENABLE_CPPCHECK`, `security-scan` preset |
| **include-what-you-use** | `brew install include-what-you-use` / `apt install iwyu` | `ENABLE_IWYU` |

### Runtime Analysis

| Tool | Install | Used By |
|------|---------|---------|
| **Valgrind** | `apt install valgrind` (Linux only) | `ENABLE_VALGRIND` |
| **Sanitizers** | Built into GCC/Clang (MSan: Clang only) | `asan`, `ubsan`, `tsan`, `msan` presets |

> See [SECURITY_SCANNING.md](SECURITY_SCANNING.md) for detailed usage of all analysis tools.

### Code Formatting

| Tool | Install | Config |
|------|---------|--------|
| **clang-format** | Part of `clang-tools-extra` | `.clang-format` (LLVM-based, 4-space indent, 100-col limit) |

```bash
# Format all source files
find include src examples tests -name '*.cpp' -o -name '*.hpp' -o -name '*.h' | xargs clang-format -i
```

### Documentation

| Tool | Install | Config |
|------|---------|--------|
| **Doxygen** | `brew install doxygen` / `apt install doxygen` | `Doxyfile` |

```bash
# Via CMake
cmake --preset default -DBUILD_DOCS=ON
cmake --build build/default --target docs

# Or directly
doxygen Doxyfile
# Output: build/docs/html/index.html
```

---

## Dependencies

All managed by vcpkg (auto-installed on first configure via `vcpkg.json` manifest mode).

### Core Dependencies

| Package | vcpkg Name | Purpose |
|---------|-----------|---------|
| fmt | `fmt` | String formatting (`HAS_FMT`) |
| spdlog | `spdlog` | Logging (`HAS_SPDLOG`) |
| nlohmann/json | `nlohmann-json` | JSON parsing (`HAS_JSON`) |
| Catch2 3.x | `catch2` | Unit testing |
| Google Benchmark | `benchmark` | Microbenchmarks |
| cpp-httplib | `cpp-httplib` | HTTP server/client (api module) |
| SQLite3 | `sqlite3` | Database (database module) |
| CLI11 2.4.2 | *(FetchContent)* | CLI argument parsing |

### Optional Feature Dependencies

Enable via `vcpkg install --x-feature=<feature>` or vcpkg manifest features:

| Feature | Packages | Purpose |
|---------|----------|---------|
| `rendering` | `glfw3`, `glm`, `glad` | OpenGL rendering module |
| `boost` | `boost-asio`, `boost-beast`, `boost-program-options` | Boost libraries (`HAS_BOOST`) |

---

## IDE Setup

### VS Code

Recommended extensions:
- **C/C++** (`ms-vscode.cpptools`) — IntelliSense, debugging
- **CMake Tools** (`ms-vscode.cmake-tools`) — preset integration, build/test UI
- **clangd** (`llvm-vs-code-extensions.vscode-clangd`) — alternative to cpptools, uses `compile_commands.json`

`.vscode/settings.json`:
```json
{
    "cmake.configureOnOpen": true,
    "cmake.sourceDirectory": "${workspaceFolder}",
    "C_Cpp.default.cppStandard": "c++20",
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/default/compile_commands.json"
}
```

> CMake Tools auto-detects presets from `CMakePresets.json`. Select a preset via the status bar.

### CLion

1. Open the project root (CLion auto-detects `CMakePresets.json`)
2. Go to **Settings → Build → CMake** — presets appear automatically
3. Select `default` or `debug` profile
4. CLion uses `compile_commands.json` for code analysis

### Visual Studio 2022

1. **File → Open → CMake** — select `CMakeLists.txt`
2. VS auto-detects `CMakePresets.json` and shows presets in the toolbar
3. Set `VCPKG_ROOT` environment variable before opening

---

## Verifying Your Setup

```bash
# Check compiler
g++ --version    # or clang++ --version, cl (MSVC)

# Check CMake
cmake --version  # must be >= 3.21

# Check vcpkg
echo $VCPKG_ROOT  # should point to vcpkg installation

# Configure and build
cmake --preset default
cmake --build build/default

# Run tests
ctest --test-dir build/default --output-on-failure

# Optional: static analysis
cmake --preset security-scan
cmake --build build/security-scan
```

If any dependency is missing, vcpkg will download and build it automatically during the configure step.
