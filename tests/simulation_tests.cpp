/// @file simulation_tests.cpp
/// @brief Unit tests for Simpson's rule and RK4 ODE solver.

#include "simulation/ecs.h"
#include "simulation/numerical.h"
#include "simulation/physics.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <numbers>
#include <stdexcept>

using Catch::Matchers::WithinAbs;
using namespace simulation;

TEST_CASE("Simpson's rule integrates polynomials exactly", "[simulation][numerical]") {
    // ∫₀¹ x² dx = 1/3
    auto result = simpson([](double x) { return x * x; }, 0.0, 1.0, 1000);
    REQUIRE_THAT(result, WithinAbs(1.0 / 3.0, 1e-10));
}

TEST_CASE("Simpson's rule integrates sin(x) over [0, pi]", "[simulation][numerical]") {
    // ∫₀^π sin(x) dx = 2
    auto result = simpson([](double x) { return std::sin(x); }, 0.0, std::numbers::pi, 1000);
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

TEST_CASE("RK4 rejects non-positive dt", "[simulation][numerical]") {
    REQUIRE_THROWS_AS(rk4([](double, double y) { return y; }, 1.0, 0.0, 1.0, 0.0),
                      std::invalid_argument);
}

TEST_CASE("PhysicsWorld distance constraint", "[simulation][physics]") {
    PhysicsWorld world;
    world.gravity = {0.0, 0.0};
    auto a = world.add_particle({0.0, 0.0}, 1.0, true);
    auto b = world.add_particle({2.0, 0.0});
    world.add_constraint(a, b);
    REQUIRE_THROWS_AS(world.add_constraint(0, 99), std::out_of_range);
    world.step(0.016);
    auto dist = (world.particles()[b].pos - world.particles()[a].pos).length();
    REQUIRE_THAT(dist, WithinAbs(2.0, 1e-6));
}

TEST_CASE("ECS add/get/each", "[simulation][ecs]") {
    struct Pos {
        double x{};
    };
    struct Vel {
        double x{};
    };
    World world;
    auto e = world.create();
    world.add(e, Pos{1.0});
    world.add(e, Vel{2.0});
    REQUIRE(world.get<Pos>(e)->x == 1.0);
    int seen = 0;
    world.each<Pos, Vel>([&](Entity, Pos &p, Vel &v) {
        p.x += v.x;
        ++seen;
    });
    REQUIRE(seen == 1);
    REQUIRE(world.get<Pos>(e)->x == 3.0);
    world.remove<Vel>(e);
    REQUIRE(world.get<Vel>(e) == nullptr);
    world.add(e, Vel{1.0});
    world.destroy(e);
    REQUIRE(world.get<Pos>(e) == nullptr);
    REQUIRE(world.get<Vel>(e) == nullptr);
}
