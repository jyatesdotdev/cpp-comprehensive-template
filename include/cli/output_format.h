#pragma once
/// @file output_format.h
/// @brief Terminal output helpers: ANSI colors, table formatting, progress bar.

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace cli::fmt {

// ── ANSI Color Codes ────────────────────────────────────────────────

/// @brief ANSI escape code constants for terminal coloring.
namespace color {
inline constexpr const char* reset   = "\033[0m";   ///< @brief Reset all attributes.
inline constexpr const char* bold    = "\033[1m";   ///< @brief Bold/bright text.
inline constexpr const char* dim     = "\033[2m";   ///< @brief Dim/faint text.
inline constexpr const char* red     = "\033[31m";  ///< @brief Red foreground.
inline constexpr const char* green   = "\033[32m";  ///< @brief Green foreground.
inline constexpr const char* yellow  = "\033[33m";  ///< @brief Yellow foreground.
inline constexpr const char* blue    = "\033[34m";  ///< @brief Blue foreground.
inline constexpr const char* magenta = "\033[35m";  ///< @brief Magenta foreground.
inline constexpr const char* cyan    = "\033[36m";  ///< @brief Cyan foreground.
inline constexpr const char* white   = "\033[37m";  ///< @brief White foreground.
} // namespace color

/// @brief Wrap text in an ANSI color code.
/// @param text The string to colorize.
/// @param code ANSI escape code (e.g. @c color::red).
/// @return A new string with the color code prepended and reset appended.
inline std::string colorize(const std::string& text, const char* code) {
    return std::string(code) + text + color::reset;
}

// ── Visible Length (strips ANSI escapes) ─────────────────────────────

/// @brief Returns the visible character count, ignoring ANSI escape sequences.
/// @param s The string (may contain ANSI escapes).
/// @return Number of visible (non-escape) characters.
inline size_t visible_length(const std::string& s) {
    size_t len = 0;
    bool in_esc = false;
    for (char c : s) {
        if (in_esc) { if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) in_esc = false; }
        else if (c == '\033') { in_esc = true; }
        else { ++len; }
    }
    return len;
}

// ── Table Formatting ────────────────────────────────────────────────

/// @brief Simple column-aligned table for terminal output.
///
/// Handles ANSI-colored cell values correctly.
/// @code
///   Table t({"Name", "Port", "Status"});
///   t.add_row({"api", "8080", colorize("running", color::green)});
///   t.print();
/// @endcode
class Table {
public:
    /// @brief Construct a table with the given column headers.
    /// @param headers Column header strings.
    explicit Table(std::vector<std::string> headers)
        : headers_(std::move(headers)), widths_(headers_.size(), 0) {
        for (size_t i = 0; i < headers_.size(); ++i)
            widths_[i] = visible_length(headers_[i]);
    }

    /// @brief Append a row of values. Short rows are padded with empty strings.
    /// @param row Cell values for this row.
    void add_row(std::vector<std::string> row) {
        row.resize(headers_.size()); // pad if short
        for (size_t i = 0; i < row.size(); ++i)
            widths_[i] = std::max(widths_[i], visible_length(row[i]));
        rows_.push_back(std::move(row));
    }

    /// @brief Print the formatted table to an output stream.
    /// @param os Output stream (defaults to @c std::cout).
    void print(std::ostream& os = std::cout) const {
        auto print_row = [&](const std::vector<std::string>& row) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0) os << "  ";
                os << row[i];
                if (i + 1 < row.size()) {
                    auto vlen = visible_length(row[i]);
                    if (vlen < widths_[i])
                        os << std::string(widths_[i] - vlen, ' ');
                }
            }
            os << "\n";
        };
        auto separator = [&]() {
            for (size_t i = 0; i < widths_.size(); ++i) {
                if (i > 0) os << "  ";
                os << std::string(widths_[i], '-');
            }
            os << "\n";
        };

        print_row(headers_);
        separator();
        for (auto& row : rows_) print_row(row);
    }

private:
    std::vector<std::string> headers_;
    std::vector<size_t> widths_;
    std::vector<std::vector<std::string>> rows_;
};

// ── Progress Bar ────────────────────────────────────────────────────

/// @brief Renders a progress bar to stderr. Call update() in a loop.
/// @code
///   ProgressBar bar(100, "Processing");
///   for (int i = 0; i <= 100; ++i) bar.update(i);
///   bar.finish();
/// @endcode
class ProgressBar {
public:
    /// @brief Construct a progress bar.
    /// @param total Total number of steps.
    /// @param label Optional label printed before the bar.
    /// @param width Bar width in characters (default 40).
    explicit ProgressBar(int total, std::string label = "", int width = 40)
        : total_(total), label_(std::move(label)), width_(width) {}

    /// @brief Redraw the bar at the given progress.
    /// @param current Current step (0 to @p total).
    void update(int current) const {
        float frac = static_cast<float>(current) / static_cast<float>(total_);
        int filled = static_cast<int>(frac * static_cast<float>(width_));
        int pct = static_cast<int>(frac * 100.0f);
        std::cerr << "\r";
        if (!label_.empty()) std::cerr << label_ << " ";
        std::cerr << "[" << std::string(static_cast<size_t>(filled), '#')
                  << std::string(static_cast<size_t>(width_ - filled), '.')
                  << "] " << pct << "%" << std::flush;
    }

    /// @brief Complete the bar (draws at 100% and prints a newline).
    void finish() const {
        update(total_);
        std::cerr << "\n";
    }

private:
    int total_;
    std::string label_;
    int width_;
};

} // namespace cli::fmt
