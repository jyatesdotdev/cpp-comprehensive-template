/// @file cli_tests.cpp
/// @brief Tests for CLI parsing helpers, validators, and output formatting.

#include <catch2/catch_test_macros.hpp>

#include "cli/cli_helpers.h"
#include "cli/output_format.h"

#include <CLI/CLI.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

// ── Helper: write a temp config file ────────────────────────────────

/// @brief Write a temporary config file and return its path.
static std::filesystem::path write_temp_config(const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / "cli_test_config.txt";
    std::ofstream(path) << content;
    return path;
}

// ═══════════════════════════════════════════════════════════════════════
// parse_config_file
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("parse_config_file: basic key=value", "[cli]") {
    auto p = write_temp_config("host=localhost\nport=9090\n");
    auto cfg = cli::parse_config_file(p);
    CHECK(cfg["host"] == "localhost");
    CHECK(cfg["port"] == "9090");
    std::filesystem::remove(p);
}

TEST_CASE("parse_config_file: comments and blank lines", "[cli]") {
    auto p = write_temp_config("# comment\n\nkey=val\n# another\n");
    auto cfg = cli::parse_config_file(p);
    CHECK(cfg.size() == 1);
    CHECK(cfg["key"] == "val");
    std::filesystem::remove(p);
}

TEST_CASE("parse_config_file: whitespace trimming", "[cli]") {
    auto p = write_temp_config("  host  =  127.0.0.1  \n");
    auto cfg = cli::parse_config_file(p);
    CHECK(cfg["host"] == "127.0.0.1");
    std::filesystem::remove(p);
}

TEST_CASE("parse_config_file: nonexistent file returns empty", "[cli]") {
    auto cfg = cli::parse_config_file("/tmp/cli_test_nonexistent_file.cfg");
    CHECK(cfg.empty());
}

// ═══════════════════════════════════════════════════════════════════════
// Validators
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("PortRange: valid ports", "[cli]") {
    CHECK(cli::PortRange("1").empty());
    CHECK(cli::PortRange("8080").empty());
    CHECK(cli::PortRange("65535").empty());
}

TEST_CASE("PortRange: invalid ports", "[cli]") {
    CHECK_FALSE(cli::PortRange("0").empty());
    CHECK_FALSE(cli::PortRange("65536").empty());
    CHECK_FALSE(cli::PortRange("-1").empty());
    CHECK_FALSE(cli::PortRange("abc").empty());
}

TEST_CASE("NonEmpty: valid and invalid", "[cli]") {
    CHECK(cli::NonEmpty("hello").empty());
    CHECK_FALSE(cli::NonEmpty("").empty());
}

// ═══════════════════════════════════════════════════════════════════════
// apply_config_defaults
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("apply_config_defaults: sets unset options", "[cli]") {
    CLI::App app{"test"};
    std::string host = "default";
    app.add_option("--host", host);

    // Don't parse any args — option is "empty"
    app.parse(std::vector<std::string>{});

    std::unordered_map<std::string, std::string> cfg{{"host", "from-config"}};
    cli::apply_config_defaults(app, cfg);

    // Default should now be "from-config"
    CHECK(host == "from-config");
}

TEST_CASE("apply_config_defaults: CLI value takes priority", "[cli]") {
    CLI::App app{"test"};
    std::string host;
    app.add_option("--host", host);

    app.parse("--host from-cli");

    std::unordered_map<std::string, std::string> cfg{{"host", "from-config"}};
    cli::apply_config_defaults(app, cfg);

    CHECK(host == "from-cli");
}

TEST_CASE("apply_config_defaults: unknown keys are ignored", "[cli]") {
    CLI::App app{"test"};
    std::string host;
    app.add_option("--host", host);
    app.parse(std::vector<std::string>{});

    std::unordered_map<std::string, std::string> cfg{{"unknown_key", "val"}};
    cli::apply_config_defaults(app, cfg); // should not throw
}

// ═══════════════════════════════════════════════════════════════════════
// CLI11 subcommand parsing
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("CLI11: subcommand selection", "[cli]") {
    CLI::App app{"test"};
    bool serve_ran = false, info_ran = false;

    auto* serve = app.add_subcommand("serve", "");
    serve->callback([&]() { serve_ran = true; });

    auto* info = app.add_subcommand("info", "");
    info->callback([&]() { info_ran = true; });

    app.require_subcommand(1);
    app.parse(std::vector<std::string>{"serve"});

    CHECK(serve_ran);
    CHECK_FALSE(info_ran);
}

TEST_CASE("CLI11: option with validator rejects bad input", "[cli]") {
    CLI::App app{"test"};
    int port = 0;
    app.add_option("--port", port)->check(cli::PortRange);

    CHECK_THROWS_AS(
        app.parse("--port 99999"),
        CLI::ValidationError);
}

TEST_CASE("CLI11: flag parsing", "[cli]") {
    CLI::App app{"test"};
    bool verbose = false;
    app.add_flag("-v,--verbose", verbose);

    app.parse(std::vector<std::string>{"-v"});
    CHECK(verbose);
}

// ═══════════════════════════════════════════════════════════════════════
// Output formatting
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("visible_length: plain text", "[cli][fmt]") {
    CHECK(cli::fmt::visible_length("hello") == 5);
    CHECK(cli::fmt::visible_length("") == 0);
}

TEST_CASE("visible_length: strips ANSI codes", "[cli][fmt]") {
    auto colored = cli::fmt::colorize("hello", cli::fmt::color::red);
    CHECK(cli::fmt::visible_length(colored) == 5);
}

TEST_CASE("colorize: wraps text with code and reset", "[cli][fmt]") {
    auto result = cli::fmt::colorize("ok", cli::fmt::color::green);
    CHECK(result == std::string("\033[32m") + "ok" + "\033[0m");
}

TEST_CASE("Table: renders aligned output", "[cli][fmt]") {
    cli::fmt::Table t({"Name", "Value"});
    t.add_row({"a", "1"});
    t.add_row({"longer", "2"});

    std::ostringstream os;
    t.print(os);
    auto output = os.str();

    // Should contain header and separator
    CHECK(output.find("Name") != std::string::npos);
    CHECK(output.find("------") != std::string::npos);
    CHECK(output.find("longer") != std::string::npos);
}
