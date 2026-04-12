#pragma once

/// @file app.h
/// @brief Core application framework with RAII lifecycle management.

#include <string>
#include <string_view>

namespace core {

/// @brief Application configuration loaded at startup.
struct AppConfig {
    std::string name = "CppTemplate"; ///< Application display name.
    int log_level = 1;                ///< Log verbosity: 0=trace, 1=info, 2=warn, 3=error.
};

/// @brief Base application class demonstrating RAII initialization.
///
/// Uses the PImpl idiom to hide implementation details and provide a stable ABI.
/// Non-copyable, move-only.
class App {
public:
    /// @brief Construct the application with the given configuration.
    /// @param config Application configuration (uses defaults if omitted).
    explicit App(AppConfig config = {});
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) noexcept;
    App& operator=(App&&) noexcept;

    /// @brief Run the application main loop.
    /// @return Exit code (0 on success).
    int run();

    /// @brief Get the application name.
    /// @return View into the configured application name.
    [[nodiscard]] std::string_view name() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace core
