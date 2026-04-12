/// @file simulation_demo.cpp
/// @brief Demonstrates ECS, Verlet physics, and numerical computing.

#include <cmath>
#include <iomanip>
#include <iostream>

#include "simulation/ecs.h"
#include "simulation/numerical.h"
#include "simulation/physics.h"

// ── ECS demo components ─────────────────────────────────────────────
/// @brief ECS demo component — 2D position.
struct Position { double x, y; };
/// @brief ECS demo component — 2D velocity.
struct Velocity { double dx, dy; };
/// @brief ECS demo component — entity name.
struct Name { std::string value; };

int main() {
    std::cout << std::fixed << std::setprecision(4);

    // ── 1. ECS ──────────────────────────────────────────────────────
    std::cout << "=== Entity-Component-System ===\n";
    simulation::World world;
    auto e0 = world.create();
    auto e1 = world.create();
    world.add(e0, Position{0, 0});
    world.add(e0, Velocity{1, 2});
    world.add(e0, Name{"Player"});
    world.add(e1, Position{5, 5});
    world.add(e1, Name{"Static"});  // no Velocity

    // Only entities with both Position and Velocity get updated.
    world.each<Position, Velocity>([](simulation::Entity e, Position& p, Velocity& v) {
        p.x += v.dx;
        p.y += v.dy;
        std::cout << "  Entity " << e << " moved to (" << p.x << ", " << p.y << ")\n";
    });

    // ── 2. Verlet Physics ───────────────────────────────────────────
    std::cout << "\n=== Verlet Physics (2-particle spring) ===\n";
    simulation::PhysicsWorld phys;
    phys.gravity = {0, -9.81};
    auto a = phys.add_particle({0, 5}, 1.0, true);   // pinned anchor
    auto b = phys.add_particle({0, 3}, 1.0, false);   // hanging mass
    phys.add_constraint(a, b);

    for (int i = 0; i < 5; ++i) {
        phys.step(1.0 / 60.0);
        auto& p = phys.particles()[b];
        std::cout << "  step " << i << ": y=" << p.pos.y << "\n";
    }

    // ── 3. Numerical: RK4 ODE solver ────────────────────────────────
    std::cout << "\n=== RK4: dy/dt = -2y (exact: e^{-2t}) ===\n";
    auto result = simulation::rk4([](double, double y) { return -2.0 * y; },
                                  1.0, 0.0, 1.0, 0.1);
    for (auto& [t, y] : result) {
        double exact = std::exp(-2.0 * t);
        std::cout << "  t=" << t << "  y=" << y << "  exact=" << exact
                  << "  err=" << std::abs(y - exact) << "\n";
    }

    // ── 4. Numerical: Simpson's rule ────────────────────────────────
    std::cout << "\n=== Simpson: integral of sin(x) from 0 to pi ===\n";
    double integral = simulation::simpson([](double x) { return std::sin(x); }, 0, M_PI);
    std::cout << "  result=" << integral << "  exact=2.0000  err="
              << std::abs(integral - 2.0) << "\n";
}
