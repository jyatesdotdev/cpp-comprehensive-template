/// @file simulation_tests.cpp
/// @brief Unit tests for Simpson's rule and RK4 ODE solver.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include "simulation/numerical.h"

using Catch::Matchers::WithinAbs;
using namespace simulation;

TEST_CASE("Simpson's rule integrates polynomials exactly", "[simulation][numerical]") {
    // ∫₀¹ x² dx = 1/3
    auto result = simpson([](double x) { return x * x; }, 0.0, 1.0, 1000);
    REQUIRE_THAT(result, WithinAbs(1.0 / 3.0, 1e-10));
}

TEST_CASE("Simpson's rule integrates sin(x) over [0, pi]", "[simulation][numerical]") {
    // ∫₀^π sin(x) dx = 2
    auto result = simpson([](double x) { return std::sin(x); }, 0.0, M_PI, 1000);
    REQUIRE_THAT(result, WithinAbs(2.0, 1e-8));
}

TEST_CASE("RK4 solves dy/dt = y (exponential growth)", "[simulation][numerical]") {
    // dy/dt = y, y(0) = 1 → y(t) = e^t
    auto trajectory = rk4([](double, double y) { return y; }, 1.0, 0.0, 1.0, 0.001);

    REQUIRE(trajectory.size() > 1);
    auto [t_final, y_final] = trajectory.back();
    REQUIRE_THAT(t_final, WithinAbs(1.0, 0.01));
    REQUIRE_THAT(y_final, WithinAbs(std::exp(1.0), 1e-6));
}

TEST_CASE("RK4 solves dy/dt = -y (exponential decay)", "[simulation][numerical]") {
    auto trajectory = rk4([](double, double y) { return -y; }, 1.0, 0.0, 2.0, 0.001);
    auto [t, y] = trajectory.back();
    REQUIRE_THAT(y, WithinAbs(std::exp(-2.0), 1e-6));
}
