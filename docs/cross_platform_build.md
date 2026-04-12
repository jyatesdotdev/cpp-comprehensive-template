# Cross-Platform Build Instructions

How to build this project on Linux, macOS, and Windows.

## Prerequisites

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| CMake | 3.21 | `cmake --version` |
| C++ compiler | C++20 support | GCC 12+, Clang 15+, MSVC 17.4+ |
| vcpkg | latest | Optional but recommended |

## Installing vcpkg

```bash
# Linux / macOS
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg   # add to .bashrc / .zshrc

# Windows (PowerShell)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
$env:VCPKG_ROOT = "C:\vcpkg"  # add to system environment
```

## Building with Presets (Recommended)

```bash
# Configure (vcpkg auto-installs dependencies)
cmake --preset default

# Build
cmake --build build

# Test
ctest --test-dir build --output-on-failure

# Release build
cmake --preset release
cmake --build build --config Release
```

## Building without vcpkg

Install dependencies via your system package manager, then configure without the toolchain file:

```bash
# Ubuntu / Debian
sudo apt install libfmt-dev libspdlog-dev nlohmann-json3-dev \
    catch2 libsqlite3-dev libssl-dev

# macOS (Homebrew)
brew install fmt spdlog nlohmann-json catch2 sqlite

# Configure without vcpkg
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Platform-Specific Notes

### Linux

```bash
# GCC
cmake -B build -DCMAKE_CXX_COMPILER=g++-12

# Clang with libc++
cmake -B build -DCMAKE_CXX_COMPILER=clang++-15 \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++"
```

### macOS

```bash
# Apple Clang (Xcode 14.3+ for C++20)
cmake -B build

# Homebrew GCC
cmake -B build -DCMAKE_CXX_COMPILER=g++-13
```

### Windows (MSVC)

```powershell
# Visual Studio generator
cmake -B build -G "Visual Studio 17 2022" -A x64

# Ninja (faster)
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=cl
cmake --build build --config Release
```

### Windows (MinGW / MSYS2)

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

## Optional Features

| CMake Option | Default | Description |
|---|---|---|
| `BUILD_EXAMPLES` | ON | Build example programs |
| `BUILD_TESTS` | ON | Build Catch2 tests |
| `BUILD_BENCHMARKS` | OFF | Build Google Benchmark targets |
| `BUILD_DOCS` | OFF | Build Doxygen documentation |
| `ENABLE_SANITIZERS` | OFF | Enable ASan + UBSan |
| `ENABLE_RENDERING` | OFF | Build OpenGL rendering module |

```bash
# Example: enable everything
cmake --preset default \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_DOCS=ON \
    -DENABLE_RENDERING=ON \
    -DENABLE_SANITIZERS=ON
```

## Generating Documentation

```bash
# Requires Doxygen (https://www.doxygen.nl)
cmake --preset default -DBUILD_DOCS=ON
cmake --build build --target docs

# Or directly
cd template && doxygen Doxyfile
# Output: build/docs/html/index.html
```

## CI Integration

Use the `ci` preset for automated builds:

```yaml
# GitHub Actions example
- name: Configure
  run: cmake --preset ci
- name: Build
  run: cmake --build build --config Release
- name: Test
  run: ctest --test-dir build --output-on-failure
```

## Troubleshooting

| Problem | Solution |
|---|---|
| vcpkg not found | Set `VCPKG_ROOT` environment variable |
| C++20 features missing | Upgrade compiler (GCC 12+, Clang 15+, MSVC 17.4+) |
| `std::jthread` unavailable | GCC 11+ or Clang 17+ with libc++; MSVC 2022 |
| Rendering won't build | Pass `-DENABLE_RENDERING=ON` and install glfw3/glm |
| Sanitizer link errors on macOS | Use `cmake --preset debug` (sets correct flags) |
| Parallel STL missing on macOS | Apple Clang lacks `<execution>`; use GCC or install TBB |

## See Also

- [Toolchain](TOOLCHAIN.md) — Detailed tool versions, IDE setup, per-platform install
- [Tutorial](TUTORIAL.md) — Step-by-step build walkthrough for new developers
- [Architecture](ARCHITECTURE.md) — CMake targets and build options
- [Security Scanning](SECURITY_SCANNING.md) — Sanitizer presets and static analysis
