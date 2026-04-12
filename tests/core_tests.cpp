/// @file core_tests.cpp
/// @brief Unit tests for core::App initialization and run.

#include <catch2/catch_test_macros.hpp>
#include "core/app.h"

TEST_CASE("App initializes with default config", "[core]") {
    core::App app;
    REQUIRE(app.name() == "CppTemplate");
}

TEST_CASE("App initializes with custom config", "[core]") {
    core::App app({.name = "TestApp", .log_level = 3});
    REQUIRE(app.name() == "TestApp");
}

TEST_CASE("App::run returns 0", "[core]") {
    core::App app;
    REQUIRE(app.run() == 0);
}
