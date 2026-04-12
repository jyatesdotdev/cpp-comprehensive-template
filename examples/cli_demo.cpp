/// @file cli_demo.cpp
/// @brief CLI11 demo with subcommands, validators, callbacks, env fallback.
///        Also demonstrates Boost.ProgramOptions alternative (when HAS_BOOST defined)
///        and real-world config+CLI+env merging patterns.

#include "cli/cli_helpers.h"
#include "cli/output_format.h"

#include <CLI/CLI.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef HAS_BOOST
#include <boost/program_options.hpp>
#include <fstream>
namespace po = boost::program_options;

// ═══════════════════════════════════════════════════════════════════════
// Boost.ProgramOptions Alternative
// ═══════════════════════════════════════════════════════════════════════

/// Demonstrates Boost.ProgramOptions: options_description, positional args,
/// config file parsing, and multi-source merging (CLI > config > defaults).
int run_boost_demo(int argc, char** argv) {
    // ── Define options ──────────────────────────────────────────────
    po::options_description generic("Generic options");
    generic.add_options()
        ("help,h",    "Show help")
        ("verbose,v", "Enable verbose output")
        ("config,c",  po::value<std::string>(), "Config file path");

    po::options_description server("Server options");
    server.add_options()
        ("host",    po::value<std::string>()->default_value("0.0.0.0"), "Bind address")
        ("port,p",  po::value<int>()->default_value(8080),             "Listen port")
        ("workers", po::value<int>()->default_value(4),                "Worker threads");

    // ── Positional args (e.g., `mytool serve`) ──────────────────────
    po::options_description hidden("Hidden");
    hidden.add_options()
        ("command", po::value<std::string>(), "Subcommand");

    po::positional_options_description positional;
    positional.add("command", 1);

    po::options_description all;
    all.add(generic).add(server).add(hidden);

    po::options_description visible("Allowed options");
    visible.add(generic).add(server);

    // ── Parse: CLI first, then config file ──────────────────────────
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(all)
                  .positional(positional)
                  .run(),
              vm);

    // Config file as second source (CLI values take priority)
    if (vm.count("config")) {
        std::ifstream ifs(vm["config"].as<std::string>());
        if (ifs) po::store(po::parse_config_file(ifs, server), vm);
    }
    po::notify(vm);

    if (vm.count("help") || !vm.count("command")) {
        std::cout << "Usage: cli_demo boost [options]\n" << visible << "\n";
        return 0;
    }

    auto cmd = vm["command"].as<std::string>();
    if (cmd == "serve") {
        std::cout << "Boost: serving on " << vm["host"].as<std::string>()
                  << ":" << vm["port"].as<int>()
                  << " workers=" << vm["workers"].as<int>() << "\n";
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
    }
    return 0;
}
#endif // HAS_BOOST

// ═══════════════════════════════════════════════════════════════════════
// Real-world pattern: config + CLI + env merging with priority
// ═══════════════════════════════════════════════════════════════════════

/// Demonstrates the common pattern: CLI flags > env vars > config file > defaults.
/// Uses CLI11's built-in envname() plus manual config-file fallback.
/// Takes pointer to config_file so callback reads the value after CLI parsing.
void setup_merged_serve(CLI::App& app, const std::string* config_file_ptr) {
    struct ServeOpts {
        std::string host    = "0.0.0.0";
        int         port    = 8080;
        int         workers = 4;
        std::string log_level = "info";
    };
    static ServeOpts opts;

    auto* serve = app.add_subcommand("serve", "Start the server");

    // Priority chain: CLI flag > env var > config file > default
    serve->add_option("--host", opts.host, "Bind address")
        ->envname("MYTOOL_HOST");
    serve->add_option("-p,--port", opts.port, "Listen port")
        ->envname("MYTOOL_PORT")
        ->check(cli::PortRange);
    serve->add_option("-w,--workers", opts.workers, "Worker threads")
        ->envname("MYTOOL_WORKERS")
        ->check(CLI::Range(1, 64));
    serve->add_option("--log-level", opts.log_level, "Log level")
        ->envname("MYTOOL_LOG_LEVEL")
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error"}));

    static bool dry_run = false;
    serve->add_flag("--dry-run", dry_run, "Validate without starting");

    serve->callback([serve, config_file_ptr]() {
        // Apply config file as lowest-priority fallback (after CLI + env)
        if (config_file_ptr && !config_file_ptr->empty()) {
            auto cfg = cli::parse_config_file(*config_file_ptr);
            cli::apply_config_defaults(*serve, cfg);
        }
        std::cout << "serve: host=" << opts.host
                  << " port=" << opts.port
                  << " workers=" << opts.workers
                  << " log_level=" << opts.log_level
                  << (dry_run ? " [dry-run]" : "") << "\n";
    });
}

// ═══════════════════════════════════════════════════════════════════════
// Main — CLI11 primary interface
// ═══════════════════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    CLI::App app{"MyTool — CLI11 demo with subcommands, validators & callbacks"};
    app.require_subcommand(1);

    // ── Global options (inherited by subcommands) ───────────────────
    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Enable verbose output")->envname("MYTOOL_VERBOSE");

    std::string config_file;
    app.add_option("-c,--config", config_file, "Path to config file");

    // ── Subcommand: serve (real-world merged config pattern) ────────
    setup_merged_serve(app, &config_file);

    // ── Subcommand: config ──────────────────────────────────────────
    auto* cfg_cmd = app.add_subcommand("config", "Manage configuration");

    auto* cfg_show = cfg_cmd->add_subcommand("show", "Show current config");
    cfg_show->callback([&]() {
        if (config_file.empty()) {
            std::cout << "No config file specified (use -c)\n";
            return;
        }
        auto cfg = cli::parse_config_file(config_file);
        for (auto& [k, v] : cfg)
            std::cout << k << " = " << v << "\n";
    });

    std::string set_key, set_val;
    auto* cfg_set = cfg_cmd->add_subcommand("set", "Set a config value");
    cfg_set->add_option("key", set_key, "Config key")->required()->check(cli::NonEmpty);
    cfg_set->add_option("value", set_val, "Config value")->required();
    cfg_set->callback([&]() {
        std::cout << "Set " << set_key << " = " << set_val << "\n";
    });

    cfg_cmd->require_subcommand(1);

    // ── Subcommand: info ────────────────────────────────────────────
    auto* info = app.add_subcommand("info", "Show build/version info");

    std::string format = "text";
    info->add_option("-f,--format", format, "Output format")
        ->check(CLI::IsMember({"text", "json"}));

    info->callback([&]() {
        if (format == "json") {
            std::cout << R"({"name":"MyTool","version":"1.0.0"})" << "\n";
        } else {
            std::cout << "MyTool v1.0.0\n";
        }
    });

    // ── Subcommand: format (output formatting demo) ───────────────
    auto* fmt_cmd = app.add_subcommand("format", "Demonstrate output formatting");
    std::string fmt_what = "all";
    fmt_cmd->add_option("what", fmt_what, "What to demo: table|color|progress|all")
        ->check(CLI::IsMember({"table", "color", "progress", "all"}));

    fmt_cmd->callback([&]() {
        using namespace cli::fmt;

        if (fmt_what == "table" || fmt_what == "all") {
            std::cout << colorize("── Table Demo ──", color::bold) << "\n";
            Table t({"Service", "Port", "Status"});
            t.add_row({"api",       "8080", colorize("running", color::green)});
            t.add_row({"worker",    "9090", colorize("running", color::green)});
            t.add_row({"scheduler", "9091", colorize("stopped", color::red)});
            t.print();
            std::cout << "\n";
        }

        if (fmt_what == "color" || fmt_what == "all") {
            std::cout << colorize("── Color Demo ──", color::bold) << "\n";
            std::cout << colorize("error: something failed", color::red) << "\n";
            std::cout << colorize("warning: check config", color::yellow) << "\n";
            std::cout << colorize("info: server started", color::cyan) << "\n";
            std::cout << colorize("success: all tests passed", color::green) << "\n\n";
        }

        if (fmt_what == "progress" || fmt_what == "all") {
            std::cout << colorize("── Progress Demo ──", color::bold) << "\n";
            ProgressBar bar(50, "Processing");
            for (int i = 0; i <= 50; ++i) {
                bar.update(i);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            bar.finish();
        }
    });

#ifdef HAS_BOOST
    // ── Subcommand: boost (alternative demo) ────────────────────────
    auto* boost_cmd = app.add_subcommand("boost", "Run Boost.ProgramOptions demo");
    boost_cmd->callback([&]() {
        // Forward remaining args to Boost demo
        run_boost_demo(argc, argv);
    });
#endif

    // ── Parse & run ─────────────────────────────────────────────────
    CLI11_PARSE(app, argc, argv);
    return 0;
}
