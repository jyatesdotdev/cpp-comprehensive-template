# C++ Comprehensive Template

A modern C++ (C++20) project template covering systems programming, high-performance computing, rendering pipelines, RESTful APIs, database interaction, and more.

## Prerequisites

- CMake ≥ 3.21
- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- [vcpkg](https://github.com/microsoft/vcpkg) (recommended)

## Quick Start

```bash
# Clone and configure
git clone <repo-url> && cd cpp-comprehensive-template

# Configure with vcpkg (writes to build/default — the preset's binaryDir)
cmake --preset default

# Build
cmake --build --preset default

# Run example
./build/default/examples/hello_world

# Run tests
ctest --preset default
```

## Build Presets

| Preset    | Description                          |
|-----------|--------------------------------------|
| `default` | Standard build with vcpkg            |
| `debug`   | Debug build (symbols, no sanitizers) |
| `release` | Optimized release build              |
| `ci`      | CI build: tests on, examples off     |

> Also available: `asan`, `ubsan`, `tsan`, `msan`, `asan-ubsan` (sanitizers),
> `security-scan` (clang-tidy + cppcheck), and `coverage`. Each configures into
> `build/<preset>`. See [Architecture](docs/ARCHITECTURE.md).

## Project Structure

```
├── CMakeLists.txt          # Root build configuration
├── CMakePresets.json        # Build presets
├── vcpkg.json               # Dependency manifest
├── include/                 # Public headers
│   ├── core/                # Core framework (RAII, app lifecycle)
│   ├── memory/              # Smart pointers, custom allocators
│   ├── concurrency/         # Threading, async, lock-free
│   ├── hpc/                 # SIMD, parallel algorithms
│   ├── etl/                 # Data pipelines, MapReduce
│   ├── api/                 # REST client/server
│   ├── database/            # SQLite, repository pattern
│   ├── patterns/            # Design patterns
│   ├── rendering/           # OpenGL pipeline and shaders (optional)
│   ├── simulation/          # Physics, numerical computing
│   └── cli/                 # CLI11 helpers, terminal output
├── src/                     # Compiled modules only (`core`, `memory`)
├── examples/                # Working example programs
├── tests/                   # Catch2 unit tests
├── docs/                    # Documentation and guides
└── cmake/                   # Custom CMake modules
```

## Dependencies (via vcpkg)

| Library        | Purpose                    |
|----------------|----------------------------|
| fmt            | Modern formatting          |
| spdlog         | Structured logging         |
| nlohmann-json  | JSON parsing (vcpkg, else FetchContent) |
| catch2         | Unit testing               |
| cpp-httplib    | HTTP client/server (vcpkg, else FetchContent) |
| sqlite3        | Embedded database          |

Optional features: `rendering` (glfw3, glm, glad), `boost` (asio, beast, program-options).

## Modules

Each module lives in its own namespace under `include/` and `src/`:

- **core** — Application lifecycle, RAII patterns, pimpl idiom
- **memory** — Smart pointer patterns, custom allocators, memory pools
- **concurrency** — std::thread, std::async, parallel algorithms, lock-free structures
- **hpc** — SIMD intrinsics, execution policies, vectorization
- **etl** — Data pipelines, parallel transforms, stream processing
- **api** — RESTful server/client with cpp-httplib, JSON serialization
- **database** — SQLite wrapper, prepared statements, repository pattern
- **patterns** — CRTP, type erasure, visitor, observer
- **rendering** — OpenGL pipeline, shader management (optional, `ENABLE_RENDERING`)
- **simulation** — Verlet physics, ECS, numerical integration
- **cli** — CLI11 helpers, config-file defaults, terminal formatting

## Documentation

| Guide | Description |
|-------|-------------|
| [Architecture](docs/ARCHITECTURE.md) | Project structure, module hierarchy, dependency graph, design patterns |
| [Toolchain](docs/TOOLCHAIN.md) | Required tools, versions, per-platform install, IDE setup |
| [Tutorial](docs/TUTORIAL.md) | New developer walkthrough — clone, build, test, add a feature |
| [Extending](docs/EXTENDING.md) | Adding libraries, examples, tests, dependencies, presets |
| [C++ API guidelines](docs/api_design.md) | C++ API design patterns |
| [Best Practices](docs/best_practices.md) | C++20 coding guidelines (C++23 as future reading) |
| [CLI](docs/cli.md) | Command-line interface design with CLI11 |
| [Cross-Platform Build](docs/cross_platform_build.md) | Platform-specific build instructions |
| [HPC Optimization](docs/hpc_optimization.md) | SIMD intrinsics and parallel algorithm optimization |
| [Rendering Pipeline](docs/rendering_pipeline.md) | OpenGL pipeline (Vulkan concepts as background) |
| [Security Scanning](docs/SECURITY_SCANNING.md) | Static analysis, sanitizers, CI security |
| [Third-Party Integration](docs/third_party_integration.md) | Dependency management with vcpkg, fmt, spdlog, JSON |

## License

[MIT](LICENSE)
