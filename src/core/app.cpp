/// @file app.cpp
/// @brief Implementation of the core::App application framework.

#include "core/app.h"
#include <iostream>
#include <memory>

#ifdef HAS_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace core {

struct App::Impl {
    AppConfig config;
    explicit Impl(AppConfig cfg) : config(std::move(cfg)) {}
};

App::App(AppConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {
#ifdef HAS_SPDLOG
    spdlog::set_level(static_cast<spdlog::level::level_enum>(impl_->config.log_level));
    spdlog::info("App '{}' initialized", impl_->config.name);
#else
    std::cout << "App '" << impl_->config.name << "' initialized\n";
#endif
}

App::~App() = default;
App::App(App&&) noexcept = default;
App& App::operator=(App&&) noexcept = default;

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
