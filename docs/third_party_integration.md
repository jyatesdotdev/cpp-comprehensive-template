# Third-Party Library Integration Guide

Modern C++ projects rely on a rich ecosystem of libraries. This guide covers
integrating the most common ones using vcpkg and CMake, with patterns for
graceful fallback when dependencies are unavailable.

---

## Package Management with vcpkg

### Setup

```bash
# Clone vcpkg (one-time)
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh   # or bootstrap-vcpkg.bat on Windows

# Use manifest mode (recommended) — dependencies declared in vcpkg.json
cmake --preset=default   # vcpkg integrates via CMakePresets.json toolchain
```

### Manifest Mode (`vcpkg.json`)

```json
{
  "name": "my-project",
  "version": "1.0.0",
  "dependencies": ["fmt", "spdlog", "nlohmann-json"],
  "features": {
    "boost": {
      "description": "Boost support",
      "dependencies": ["boost-asio", "boost-beast", "boost-program-options"]
    }
  }
}
```

Features let users opt into heavier dependencies:
```bash
cmake --preset=default -DVCPKG_MANIFEST_FEATURES="boost"
```

### Alternative: Conan

```ini
# conanfile.txt
[requires]
fmt/10.2.1
spdlog/1.13.0
nlohmann_json/3.11.3

[generators]
CMakeDeps
CMakeToolchain
```

```bash
conan install . --output-folder=build --build=missing
cmake --preset conan-release
```

---

## fmt — Modern Formatting

`{fmt}` provides Python-style formatting, adopted as the basis for `std::format`
(C++20). Use it for type-safe, fast string formatting.

### CMake Integration

```cmake
find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
    target_link_libraries(mylib PUBLIC fmt::fmt)
    target_compile_definitions(mylib PUBLIC HAS_FMT)
endif()
```

### Usage Patterns

```cpp
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>

// Basic formatting
std::string msg = fmt::format("Hello, {}!", name);

// Positional and named arguments
fmt::print("{0} is {1} years old\n", name, age);

// Chrono formatting
auto now = std::chrono::system_clock::now();
fmt::print("Time: {:%Y-%m-%d %H:%M:%S}\n", now);

// Container formatting
std::vector<int> v = {1, 2, 3};
fmt::print("v = {}\n", v);  // v = [1, 2, 3]

// Custom type formatting
template<> struct fmt::formatter<Point> : fmt::formatter<string_view> {
    auto format(const Point& p, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};
```

### Portability Wrapper

```cpp
#ifdef HAS_FMT
  #include <fmt/core.h>
  #define LOG_FMT(msg, ...) fmt::print(msg "\n", __VA_ARGS__)
#elif __cpp_lib_format
  #include <format>
  #include <print>
  #define LOG_FMT(msg, ...) std::println(msg, __VA_ARGS__)
#else
  #include <cstdio>
  #define LOG_FMT(msg, ...) std::printf(msg "\n", __VA_ARGS__)
#endif
```

---

## spdlog — Fast Logging

spdlog is a header-only (or compiled) logging library built on top of fmt.

### CMake Integration

```cmake
find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
    target_link_libraries(mylib PUBLIC spdlog::spdlog)
    target_compile_definitions(mylib PUBLIC HAS_SPDLOG)
endif()
```

### Usage Patterns

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// Default logger (stdout, colored)
spdlog::info("Welcome to spdlog!");
spdlog::warn("Easy padding: {:>8}", 42);
spdlog::error("Error code: {}", errno);

// Named loggers with multiple sinks
auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("app.log");
auto logger = std::make_shared<spdlog::logger>("multi",
    spdlog::sinks_init_list{console_sink, file_sink});
spdlog::register_logger(logger);

// Set log level
spdlog::set_level(spdlog::level::debug);

// Custom pattern
spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
```

### Logger Factory Pattern

```cpp
class LoggerFactory {
public:
    static spdlog::logger& get(const std::string& name) {
        auto existing = spdlog::get(name);
        if (existing) return *existing;
        auto logger = spdlog::stdout_color_mt(name);
        logger->set_level(spdlog::level::debug);
        return *logger;
    }
};
```

---

## nlohmann/json — JSON for Modern C++

### CMake Integration

```cmake
find_package(nlohmann_json CONFIG QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(mylib PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(mylib PUBLIC HAS_JSON)
endif()
```

### Usage Patterns

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Construction
json j = {{"name", "Alice"}, {"age", 30}, {"scores", {95, 87, 92}}};

// Parse from string
auto j2 = json::parse(R"({"key": "value"})");

// Structured bindings (C++17)
for (auto& [key, val] : j.items()) { /* ... */ }

// Serialization
std::string s = j.dump(2);  // pretty-print with indent=2
```

### Automatic Struct Serialization

```cpp
struct Config {
    std::string host;
    int port;
    bool verbose;
};

// Macro for automatic to_json/from_json
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, host, port, verbose)

// Usage
Config cfg{"localhost", 8080, true};
json j = cfg;                       // serialize
auto cfg2 = j.get<Config>();        // deserialize
```

### Safe Access Patterns

```cpp
// value() with default — no exceptions
std::string name = j.value("name", "unknown");
int port = j.value("port", 8080);

// contains() check
if (j.contains("optional_field")) { /* ... */ }

// JSON Pointer
int nested = j.at("/config/database/port"_json_pointer);
```

---

## Boost Libraries

Boost is a large collection of peer-reviewed C++ libraries. Use vcpkg features
to pull in only what you need.

### CMake Integration

```cmake
find_package(Boost CONFIG QUIET COMPONENTS program_options)
if(Boost_FOUND)
    target_link_libraries(mylib PUBLIC Boost::program_options)
    target_compile_definitions(mylib PUBLIC HAS_BOOST)
endif()
```

### Boost.Asio — Asynchronous I/O

```cpp
#include <boost/asio.hpp>
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Simple TCP echo server
asio::io_context io;
tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 9999));

tcp::socket socket(io);
acceptor.accept(socket);

std::array<char, 1024> buf;
auto n = socket.read_some(asio::buffer(buf));
asio::write(socket, asio::buffer(buf, n));
```

### Boost.ProgramOptions — CLI Argument Parsing

```cpp
#include <boost/program_options.hpp>
namespace po = boost::program_options;

po::options_description desc("Options");
desc.add_options()
    ("help,h", "Show help")
    ("port,p", po::value<int>()->default_value(8080), "Port number")
    ("verbose,v", po::bool_switch(), "Verbose output");

po::variables_map vm;
po::store(po::parse_command_line(argc, argv, desc), vm);
po::notify(vm);

if (vm.count("help")) { std::cout << desc; return 0; }
int port = vm["port"].as<int>();
```

---

## Integration Patterns

### 1. Conditional Compilation with Feature Macros

The template uses `HAS_FMT`, `HAS_SPDLOG`, `HAS_JSON` macros set by CMake
when dependencies are found. This allows graceful degradation:

```cpp
#ifdef HAS_SPDLOG
  #include <spdlog/spdlog.h>
  #define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#else
  #include <iostream>
  #define LOG_INFO(...) std::cout << "[INFO] " << __VA_ARGS__ << "\n"
#endif
```

### 2. Interface Wrapping

Wrap third-party APIs behind your own interfaces to isolate dependencies:

```cpp
// logging.h — your interface
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(std::string_view msg) = 0;
    virtual void error(std::string_view msg) = 0;
};

// spdlog_logger.h — implementation (only compiled when spdlog available)
class SpdlogLogger : public ILogger {
    std::shared_ptr<spdlog::logger> logger_;
public:
    explicit SpdlogLogger(const std::string& name)
        : logger_(spdlog::stdout_color_mt(name)) {}
    void info(std::string_view msg) override { logger_->info(msg); }
    void error(std::string_view msg) override { logger_->error(msg); }
};
```

### 3. vcpkg Feature Gating in CMake

```cmake
# Only build Boost-dependent code when Boost is available
if(Boost_FOUND)
    add_executable(boost_demo examples/boost_demo.cpp)
    target_link_libraries(boost_demo PRIVATE Boost::program_options)
endif()
```

### 4. Version Detection

```cmake
if(fmt_FOUND)
    message(STATUS "fmt version: ${fmt_VERSION}")
    if(fmt_VERSION VERSION_GREATER_EQUAL "10.0.0")
        target_compile_definitions(mylib PUBLIC FMT_V10_PLUS)
    endif()
endif()
```

---

## Library Comparison Quick Reference

| Need | Library | vcpkg Name | Header-Only? |
|------|---------|------------|-------------|
| Formatting | fmt | `fmt` | Optional |
| Logging | spdlog | `spdlog` | Optional |
| JSON | nlohmann/json | `nlohmann-json` | Yes |
| HTTP Client/Server | cpp-httplib | `cpp-httplib` | Yes |
| Async I/O | Boost.Asio | `boost-asio` | Yes |
| CLI Args | Boost.ProgramOptions | `boost-program-options` | No |
| HTTP/WebSocket | Boost.Beast | `boost-beast` | Yes |
| Database | SQLite3 | `sqlite3` | No |
| Testing | Catch2 | `catch2` | No |
| Benchmarking | Google Benchmark | `benchmark` | No |
| Math/Graphics | GLM | `glm` | Yes |

---

## Adding a New Dependency

1. Add to `vcpkg.json` dependencies (or a feature)
2. Add `find_package()` in root `CMakeLists.txt`
3. Conditionally link with `target_link_libraries()`
4. Set a `HAS_*` compile definition for conditional compilation
5. Wrap behind an interface if the dependency might change

```cmake
# Template for adding a new optional dependency
find_package(NewLib CONFIG QUIET)
if(NewLib_FOUND)
    target_link_libraries(core PUBLIC NewLib::NewLib)
    target_compile_definitions(core PUBLIC HAS_NEWLIB)
    message(STATUS "Found NewLib ${NewLib_VERSION}")
endif()
```

## See Also

- [Extending](EXTENDING.md) — Adding vcpkg and FetchContent dependencies step-by-step
- [Architecture](ARCHITECTURE.md) — Full dependency graph and conditional targets
- [Toolchain](TOOLCHAIN.md) — vcpkg installation and setup
- [Cross-Platform Build](cross_platform_build.md) — Building without vcpkg
