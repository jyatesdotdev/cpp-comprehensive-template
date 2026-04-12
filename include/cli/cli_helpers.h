#pragma once
/// @file cli_helpers.h
/// @brief Reusable CLI helpers: env-var fallback, config merging, custom validators.

#include <CLI/CLI.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace cli {

// ── Environment Variable Fallback ───────────────────────────────────

/// @brief Set an option's default from an environment variable.
///
/// Call after @c app.add_option() to provide env-var fallback.
/// @param opt Pointer to the CLI option to modify.
/// @param env_var Name of the environment variable to read.
inline void env_fallback(CLI::Option* opt, const std::string& env_var) {
    if (const char* val = std::getenv(env_var.c_str())) {
        opt->default_val(std::string(val));
    }
}

// ── Config File + CLI Merging ───────────────────────────────────────

/// @brief Simple key=value config parser.
///
/// Lines starting with '#' are comments. Blank lines are skipped.
/// @param path Path to the config file.
/// @return Map of key-value pairs parsed from the file.
inline std::unordered_map<std::string, std::string>
parse_config_file(const std::filesystem::path& path) {
    std::unordered_map<std::string, std::string> cfg;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (auto pos = line.find('='); pos != std::string::npos) {
            auto key = line.substr(0, pos);
            auto val = line.substr(pos + 1);
            // trim whitespace
            auto trim = [](std::string& s) {
                auto b = s.find_first_not_of(" \t");
                auto e = s.find_last_not_of(" \t");
                s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
            };
            trim(key);
            trim(val);
            cfg[key] = val;
        }
    }
    return cfg;
}

/// @brief Apply config-file defaults to a CLI::App.
///
/// For each key in the config, if a matching option exists and wasn't set
/// on the command line, set its default.
/// @param app The CLI11 application to update.
/// @param cfg Key-value map from @c parse_config_file().
inline void apply_config_defaults(CLI::App& app,
                                  const std::unordered_map<std::string, std::string>& cfg) {
    for (auto& [key, val] : cfg) {
        std::string opt_name = "--" + key;
        try {
            if (auto* opt = app.get_option(opt_name); opt && opt->empty()) {
                opt->default_val(val);
            }
        } catch (const CLI::OptionNotFound&) {
            // config key has no matching CLI option — skip
        }
    }
}

// ── Custom Validators ───────────────────────────────────────────────

/// @brief Validates that a value is a valid TCP/UDP port (1–65535).
inline CLI::Validator PortRange{"PORT", [](const std::string& input) -> std::string {
    try {
        int port = std::stoi(input);
        if (port < 1 || port > 65535)
            return "Port must be 1-65535, got " + input;
    } catch (...) {
        return "Invalid port number: " + input;
    }
    return {};
}};

/// @brief Validates that a path exists and is a regular file.
inline CLI::Validator FileExists{"FILE",
                                 CLI::ExistingFile.description("FILE")};

/// @brief Validates that a string is non-empty.
inline CLI::Validator NonEmpty{"NON_EMPTY", [](const std::string& input) -> std::string {
    if (input.empty()) return "Value must not be empty";
    return {};
}};

} // namespace cli
