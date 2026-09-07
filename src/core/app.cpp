/// @file app.cpp
/// @brief Implementation of the core::App application framework.

#include "core/app.h"

#include <iostream>
#include <memory>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace core {
namespace {

#ifdef HAS_SPDLOG
/// Map AppConfig::log_level (0=trace, 1=info, 2=warn, 3=error) to spdlog.
/// spdlog's own enum is 0=trace, 1=debug, 2=info, 3=warn, 4=err — do not cast.
spdlog::level::level_enum to_spdlog_level(int log_level) noexcept {
    switch (log_level) {
    case 0:
        return spdlog::level::trace;
    case 1:
        return spdlog::level::info;
    case 2:
        return spdlog::level::warn;
    case 3:
        return spdlog::level::err;
    default:
        return spdlog::level::info;
    }
}
#endif

} // namespace

struct App::Impl {
    AppConfig config;
    explicit Impl(AppConfig cfg) : config(std::move(cfg)) {}
};

App::App(AppConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {
#ifdef HAS_SPDLOG
    spdlog::set_level(to_spdlog_level(impl_->config.log_level));
    spdlog::info("App '{}' initialized", impl_->config.name);
#else
    std::cout << "App '" << impl_->config.name << "' initialized\n";
#endif
}

App::~App() = default;
App::App(App &&) noexcept = default;
App &App::operator=(App &&) noexcept = default;

int App::run() {
#ifdef HAS_SPDLOG
    spdlog::info("App '{}' running", impl_->config.name);
#else
    std::cout << "App '" << impl_->config.name << "' running\n";
#endif
    return 0;
}

std::string_view App::name() const noexcept {
    return impl_->config.name;
}

} // namespace core
