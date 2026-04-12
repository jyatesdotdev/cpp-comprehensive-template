/// @file third_party_demo.cpp
/// @brief Demonstrates fmt, spdlog, and nlohmann/json integration patterns.

#include <iostream>
#include <string>
#include <vector>

// ── fmt ─────────────────────────────────────────────────────────────
#ifdef HAS_FMT
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>
#endif

// ── spdlog ──────────────────────────────────────────────────────────
#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

// ── nlohmann/json ───────────────────────────────────────────────────
#ifdef HAS_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

// ── Custom type for serialization demo ──────────────────────────────
/// @brief Demo configuration struct for serialization examples.
struct Config {
    std::string host = "localhost";
    int port = 8080;
    bool verbose = false;
    std::vector<std::string> modules = {"core", "api"};
};

#ifdef HAS_JSON
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, host, port, verbose, modules)
#endif

#ifdef HAS_FMT
template<> struct fmt::formatter<Config> : fmt::formatter<std::string> {
    auto format(const Config& c, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "Config{{{}:{}, verbose={}}}", c.host, c.port, c.verbose);
    }
};
#endif

// ── Demo functions ──────────────────────────────────────────────────

/// @brief Demonstrate fmt formatting, ranges, chrono, and custom types.
void demo_fmt() {
    std::cout << "\n=== fmt Demo ===\n";
#ifdef HAS_FMT
    fmt::print("Formatted int: {:>10}\n", 42);
    fmt::print("Hex: {:#x}, Oct: {:#o}, Bin: {:#b}\n", 255, 255, 255);

    std::vector<int> v = {1, 2, 3, 4, 5};
    fmt::print("Vector: {}\n", v);

    auto now = std::chrono::system_clock::now();
    fmt::print("Now: {:%Y-%m-%d %H:%M:%S}\n", now);

    Config cfg;
    fmt::print("Config: {}\n", cfg);
#else
    std::cout << "(fmt not available — install via vcpkg)\n";
#endif
}

/// @brief Demonstrate spdlog colored console logging at various levels.
void demo_spdlog() {
    std::cout << "\n=== spdlog Demo ===\n";
#ifdef HAS_SPDLOG
    auto logger = spdlog::stdout_color_mt("demo");
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

    logger->trace("Trace message");
    logger->debug("Debug: x = {}", 42);
    logger->info("Application started on port {}", 8080);
    logger->warn("Memory usage at {}%", 85);
    logger->error("Failed to open file: {}", "config.json");

    spdlog::drop("demo");
#else
    std::cout << "(spdlog not available — install via vcpkg)\n";
#endif
}

/// @brief Demonstrate nlohmann/json construction, parsing, and struct round-trip.
void demo_json() {
    std::cout << "\n=== nlohmann/json Demo ===\n";
#ifdef HAS_JSON
    // Construction
    json j = {{"name", "CppTemplate"}, {"version", 1}, {"features", {"fmt", "spdlog"}}};
    std::cout << "Constructed: " << j.dump(2) << "\n";

    // Parse
    auto parsed = json::parse(R"({"status": "ok", "code": 200})");
    std::cout << "Status: " << parsed.value("status", "unknown") << "\n";

    // Struct serialization
    Config cfg{.host = "0.0.0.0", .port = 9090, .verbose = true, .modules = {"core", "hpc"}};
    json cfg_json = cfg;
    std::cout << "Config JSON: " << cfg_json.dump(2) << "\n";

    // Round-trip
    auto cfg2 = cfg_json.get<Config>();
    std::cout << "Round-trip host: " << cfg2.host << ", port: " << cfg2.port << "\n";

    // JSON Pointer access
    json nested = {{"db", {{"host", "localhost"}, {"port", 5432}}}};
    int db_port = nested.at("/db/port"_json_pointer);
    std::cout << "DB port (via pointer): " << db_port << "\n";
#else
    std::cout << "(nlohmann/json not available — install via vcpkg)\n";
#endif
}

int main() {
    std::cout << "Third-Party Integration Demo\n";
    std::cout << "============================\n";
    std::cout << "Available libraries:";
#ifdef HAS_FMT
    std::cout << " [fmt]";
#endif
#ifdef HAS_SPDLOG
    std::cout << " [spdlog]";
#endif
#ifdef HAS_JSON
    std::cout << " [nlohmann/json]";
#endif
    std::cout << "\n";

    demo_fmt();
    demo_spdlog();
    demo_json();

    return 0;
}
