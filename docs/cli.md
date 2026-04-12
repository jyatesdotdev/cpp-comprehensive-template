# CLI Development Guide

Patterns and best practices for building command-line interfaces in C++, using CLI11 as the primary framework with Boost.ProgramOptions as an alternative.

## Table of Contents

- [Framework Comparison: CLI11 vs Boost.ProgramOptions](#framework-comparison)
- [CLI11 Quick Start](#cli11-quick-start)
- [Subcommands](#subcommands)
- [Options, Flags, and Validators](#options-flags-and-validators)
- [Environment Variable Fallback](#environment-variable-fallback)
- [Config File + CLI Merging](#config-file--cli-merging)
- [Boost.ProgramOptions Alternative](#boostprogramoptions-alternative)
- [Output Formatting](#output-formatting)
- [Testing CLI Logic](#testing-cli-logic)
- [Best Practices](#best-practices)

---

## Framework Comparison

| Feature | CLI11 | Boost.ProgramOptions |
|---|---|---|
| **Dependencies** | Header-only, zero deps | Requires Boost (compiled library) |
| **CMake integration** | `FetchContent` one-liner | `find_package(Boost COMPONENTS program_options)` |
| **Subcommands** | First-class, nestable | Manual (positional arg + dispatch) |
| **Validators** | Built-in + custom lambdas | Manual validation after parse |
| **Env var support** | Built-in `->envname()` | Manual `std::getenv()` |
| **Config files** | Built-in INI/TOML support | `parse_config_file()` for INI |
| **Type safety** | Automatic from bound variable | `as<T>()` casts at access time |
| **Error messages** | Automatic, descriptive | Requires manual formatting |
| **Learning curve** | Low — fluent API | Moderate — more boilerplate |

**Recommendation:** Use CLI11 for new projects. It's header-only, has a cleaner API, and handles subcommands natively. Use Boost.ProgramOptions when your project already depends on Boost or you need its config-file format support.

---

## CLI11 Quick Start

### CMake Setup

Add CLI11 via FetchContent in your root `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG        v2.4.2)
FetchContent_MakeAvailable(cli11)

# Create an interface library for CLI helpers
add_library(cli INTERFACE)
target_link_libraries(cli INTERFACE CLI11::CLI11)
target_include_directories(cli INTERFACE ${PROJECT_SOURCE_DIR}/include)
```

### Minimal Example

```cpp
#include <CLI/CLI.hpp>
#include <iostream>

int main(int argc, char** argv) {
    CLI::App app{"My tool description"};

    std::string name = "world";
    app.add_option("-n,--name", name, "Who to greet");

    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Verbose output");

    CLI11_PARSE(app, argc, argv);

    std::cout << "Hello, " << name << "!\n";
    if (verbose) std::cout << "(verbose mode)\n";
}
```

---

## Subcommands

CLI11 supports nested subcommands with independent options and callbacks:

```cpp
CLI::App app{"mytool"};
app.require_subcommand(1); // exactly one subcommand required

auto* serve = app.add_subcommand("serve", "Start the server");
int port = 8080;
serve->add_option("-p,--port", port, "Listen port");
serve->callback([&]() {
    std::cout << "Serving on port " << port << "\n";
});

// Nested subcommands
auto* config = app.add_subcommand("config", "Manage configuration");
config->require_subcommand(1);

auto* cfg_show = config->add_subcommand("show", "Show config");
cfg_show->callback([&]() { /* ... */ });

auto* cfg_set = config->add_subcommand("set", "Set a value");
std::string key, val;
cfg_set->add_option("key", key)->required();
cfg_set->add_option("value", val)->required();
cfg_set->callback([&]() { /* ... */ });

CLI11_PARSE(app, argc, argv);
```

Usage: `mytool serve -p 9090` or `mytool config set host 0.0.0.0`

---

## Options, Flags, and Validators

### Built-in Validators

```cpp
app.add_option("--port", port)->check(CLI::Range(1, 65535));
app.add_option("--format", fmt)->check(CLI::IsMember({"json", "text", "csv"}));
app.add_option("--input", path)->check(CLI::ExistingFile);
app.add_option("--outdir", dir)->check(CLI::ExistingDirectory);
```

### Custom Validators

Define reusable validators as `CLI::Validator` objects:

```cpp
// In include/cli/cli_helpers.h
inline CLI::Validator PortRange{"PORT", [](const std::string& input) -> std::string {
    try {
        int port = std::stoi(input);
        if (port < 1 || port > 65535)
            return "Port must be 1-65535, got " + input;
    } catch (...) {
        return "Invalid port number: " + input;
    }
    return {}; // empty string = valid
}};

inline CLI::Validator NonEmpty{"NON_EMPTY", [](const std::string& input) -> std::string {
    if (input.empty()) return "Value must not be empty";
    return {};
}};
```

Usage: `serve->add_option("-p,--port", port)->check(cli::PortRange);`

### Required Options and Positional Args

```cpp
// Required named option
app.add_option("--output", output, "Output file")->required();

// Positional argument (no -- prefix)
app.add_option("input", input_file, "Input file")->required();

// Optional positional with default
app.add_option("format", fmt, "Output format")->default_val("text");
```

---

## Environment Variable Fallback

CLI11 has built-in env var support via `->envname()`:

```cpp
serve->add_option("--host", host, "Bind address")
    ->envname("MYTOOL_HOST");       // reads $MYTOOL_HOST if --host not given
serve->add_option("-p,--port", port, "Listen port")
    ->envname("MYTOOL_PORT")
    ->check(cli::PortRange);
```

Priority: CLI flag > env var > default value.

For more control, use the manual helper:

```cpp
// cli::env_fallback sets the default from an env var
auto* opt = serve->add_option("--host", host, "Bind address");
cli::env_fallback(opt, "MYTOOL_HOST");
```

---

## Config File + CLI Merging

A common real-world pattern: CLI flags > env vars > config file > hardcoded defaults.

### Config File Parser

The `cli::parse_config_file()` helper reads simple `key=value` files:

```ini
# myapp.conf
host = 0.0.0.0
port = 9090
workers = 8
log-level = debug
```

### Merging Pattern

```cpp
void setup_serve(CLI::App& app, const std::string* config_path) {
    static struct { std::string host="0.0.0.0"; int port=8080; int workers=4; } opts;

    auto* serve = app.add_subcommand("serve", "Start server");
    serve->add_option("--host", opts.host)->envname("MYTOOL_HOST");
    serve->add_option("-p,--port", opts.port)->envname("MYTOOL_PORT")->check(cli::PortRange);
    serve->add_option("-w,--workers", opts.workers)->check(CLI::Range(1, 64));

    serve->callback([serve, config_path]() {
        // Apply config as lowest-priority fallback
        if (config_path && !config_path->empty()) {
            auto cfg = cli::parse_config_file(*config_path);
            cli::apply_config_defaults(*serve, cfg);
        }
        std::cout << "host=" << opts.host << " port=" << opts.port << "\n";
    });
}
```

`apply_config_defaults` only sets values for options that weren't already provided via CLI or env var (it checks `opt->empty()`).

**Important:** Pass `config_path` as a pointer, not by value. The callback runs after parsing, so a by-value capture would capture the empty initial value.

---

## Boost.ProgramOptions Alternative

When your project already uses Boost, `Boost.ProgramOptions` is a solid choice:

```cpp
#include <boost/program_options.hpp>
namespace po = boost::program_options;

int main(int argc, char** argv) {
    po::options_description desc("Options");
    desc.add_options()
        ("help,h",    "Show help")
        ("host",      po::value<std::string>()->default_value("0.0.0.0"), "Bind address")
        ("port,p",    po::value<int>()->default_value(8080),              "Listen port")
        ("workers",   po::value<int>()->default_value(4),                 "Worker threads");

    // Positional args
    po::options_description hidden;
    hidden.add_options()("command", po::value<std::string>(), "Subcommand");
    po::positional_options_description pos;
    pos.add("command", 1);

    po::options_description all;
    all.add(desc).add(hidden);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(pos).run(), vm);

    // Config file as second source (CLI takes priority)
    if (vm.count("config")) {
        std::ifstream ifs(vm["config"].as<std::string>());
        if (ifs) po::store(po::parse_config_file(ifs, desc), vm);
    }
    po::notify(vm);

    if (vm.count("help")) { std::cout << desc << "\n"; return 0; }

    std::cout << "host=" << vm["host"].as<std::string>()
              << " port=" << vm["port"].as<int>() << "\n";
}
```

Key differences from CLI11:
- Subcommands require manual dispatch (positional arg + if/else)
- Validation is manual (check values after `notify()`)
- Env vars require manual `std::getenv()` calls
- Config file parsing is built-in but only for INI format
- Multi-source merging works via multiple `po::store()` calls (first store wins)

---

## Output Formatting

The `cli::fmt` namespace (in `include/cli/output_format.h`) provides terminal output helpers.

### ANSI Colors

```cpp
using namespace cli::fmt;

std::cout << colorize("error: failed", color::red) << "\n";
std::cout << colorize("ok", color::green) << "\n";
std::cout << colorize("HEADER", color::bold) << "\n";
```

Available codes: `red`, `green`, `yellow`, `blue`, `cyan`, `magenta`, `white`, `bold`, `dim`, `reset`.

### Tables

```cpp
Table t({"Service", "Port", "Status"});
t.add_row({"api",    "8080", colorize("running", color::green)});
t.add_row({"worker", "9090", colorize("stopped", color::red)});
t.print(); // prints to stdout; pass an ostream for other targets
```

Output:
```
Service  Port  Status
-------  ----  -------
api      8080  running
worker   9090  stopped
```

The table uses `visible_length()` to correctly align columns even when cells contain ANSI escape codes.

### Progress Bar

```cpp
ProgressBar bar(100, "Indexing");
for (int i = 0; i <= 100; ++i) {
    bar.update(i);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
bar.finish();
```

Renders to stderr with `\r` for in-place updates: `Indexing [################........................] 40%`

---

## Testing CLI Logic

Test CLI parsing by constructing `CLI::App` programmatically and parsing string vectors — no subprocess needed.

### Validator Tests

```cpp
TEST_CASE("PortRange validator") {
    REQUIRE(cli::PortRange("8080").empty());   // valid
    REQUIRE(cli::PortRange("1").empty());      // valid
    REQUIRE(!cli::PortRange("0").empty());     // invalid
    REQUIRE(!cli::PortRange("65536").empty()); // invalid
    REQUIRE(!cli::PortRange("abc").empty());   // invalid
}
```

### Subcommand Parsing Tests

```cpp
TEST_CASE("Subcommand triggers callback") {
    CLI::App app;
    bool called = false;
    auto* sub = app.add_subcommand("run", "");
    sub->callback([&]() { called = true; });

    std::vector<std::string> args = {"run"};
    app.parse(args);
    REQUIRE(called);
}

TEST_CASE("Validator rejects bad input") {
    CLI::App app;
    int port = 0;
    app.add_option("--port", port)->check(cli::PortRange);

    std::vector<std::string> args = {"--port", "99999"};
    REQUIRE_THROWS_AS(app.parse(args), CLI::ValidationError);
}
```

### Config Parsing Tests

```cpp
TEST_CASE("parse_config_file") {
    auto tmp = std::filesystem::temp_directory_path() / "test.conf";
    std::ofstream(tmp) << "host = localhost\nport = 9090\n";

    auto cfg = cli::parse_config_file(tmp);
    REQUIRE(cfg["host"] == "localhost");
    REQUIRE(cfg["port"] == "9090");

    std::filesystem::remove(tmp);
}
```

### Output Formatting Tests

```cpp
TEST_CASE("visible_length strips ANSI") {
    using namespace cli::fmt;
    REQUIRE(visible_length("hello") == 5);
    REQUIRE(visible_length(colorize("hello", color::red)) == 5);
}

TEST_CASE("Table output") {
    using namespace cli::fmt;
    Table t({"A", "B"});
    t.add_row({"x", "y"});
    std::ostringstream os;
    t.print(os);
    REQUIRE(os.str().find("A") != std::string::npos);
    REQUIRE(os.str().find("x") != std::string::npos);
}
```

---

## Best Practices

1. **Use subcommands for distinct operations.** `mytool serve`, `mytool config show` — not `mytool --mode=serve`. Subcommands get their own help text and option sets.

2. **Establish a clear priority chain.** CLI flag > env var > config file > default. Document this for users. CLI11's `->envname()` + `apply_config_defaults()` implements this cleanly.

3. **Validate early.** Attach validators to options at definition time. Don't parse first and validate later — CLI11 gives users clear error messages automatically.

4. **Keep parsing separate from logic.** Parse into a struct, then pass the struct to your business logic. This makes testing easy (construct the struct directly in tests).

5. **Use `--` to separate flags from positional args.** CLI11 handles this automatically. Document it for users who pass filenames starting with `-`.

6. **Provide `--help` and `--version` automatically.** CLI11 adds `--help` by default. Add version with `app.set_version_flag("--version", "1.0.0")`.

7. **Use exit codes consistently.** 0 = success, 1 = runtime error, 2 = usage error. CLI11 throws typed exceptions (`CLI::ParseError`, `CLI::ValidationError`) that map to appropriate codes.

8. **Color output conditionally.** Check `isatty(fileno(stdout))` or provide `--no-color` / respect `NO_COLOR` env var before emitting ANSI codes.

9. **Test parsing logic without subprocesses.** Construct `CLI::App` in tests and call `app.parse(vector)`. This is fast and doesn't require building/running the actual executable.

10. **Prefer header-only libraries for CLI tools.** CLI11 and the helpers in this template are all header-only, which simplifies builds and distribution.

## See Also

- [Tutorial](TUTORIAL.md) — End-to-end walkthrough including CLI demo
- [Architecture](ARCHITECTURE.md) — CLI module in the project structure
- [Extending](EXTENDING.md) — Adding new subcommands and options
- [Third-Party Integration](third_party_integration.md) — Boost.ProgramOptions integration
