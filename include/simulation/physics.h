#pragma once
/// @file physics.h
/// @brief Verlet-integration particle physics with distance constraints.
///
/// Demonstrates a simple 2D physics simulation suitable for cloth, soft-body,
/// or basic particle effects.

#include <cmath>
#include <cstddef>
#include <vector>

namespace simulation {

/// @brief Simple 2D vector with basic arithmetic and length.
struct Vec2 {
    double x{}, y{};

    /// @brief Component-wise addition.
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    /// @brief Component-wise subtraction.
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    /// @brief Scalar multiplication.
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    /// @brief Euclidean length of the vector.
    /// @return Length as a double.
    [[nodiscard]] double length() const { return std::sqrt(x * x + y * y); }
};

/// @brief A point-mass particle for Verlet integration.
struct Particle {
    Vec2 pos;       ///< Current position.
    Vec2 prev;      ///< Previous position (Verlet stores velocity implicitly).
    Vec2 accel;     ///< Accumulated acceleration this frame.
    double mass{1}; ///< Particle mass (default 1).
    bool pinned{false}; ///< If true, particle is immovable.
};

/// @brief Distance constraint between two particles.
struct DistanceConstraint {
    std::size_t a;      ///< Index of the first particle.
    std::size_t b;      ///< Index of the second particle.
    double rest_length; ///< Target distance between the two particles.
};

/// @brief Simple Verlet physics world with particles and distance constraints.
class PhysicsWorld {
public:
    Vec2 gravity{0.0, -9.81}; ///< Global gravity acceleration.

    /// @brief Add a particle to the world.
    /// @param pos Initial position.
    /// @param mass Particle mass (default 1.0).
    /// @param pinned If true, the particle will not move (default false).
    /// @return Index of the newly added particle.
    std::size_t add_particle(Vec2 pos, double mass = 1.0, bool pinned = false) {
        particles_.push_back({pos, pos, {}, mass, pinned});
        return particles_.size() - 1;
    }

    /// @brief Add a distance constraint between two particles.
    /// @param a Index of the first particle.
    /// @param b Index of the second particle.
    void add_constraint(std::size_t a, std::size_t b) {
        double d = (particles_[a].pos - particles_[b].pos).length();
        constraints_.push_back({a, b, d});
    }

    /// @brief Advance simulation by @p dt seconds.
    /// @param dt Time step in seconds.
    /// @param constraint_iters Number of constraint relaxation iterations (default 4).
    void step(double dt, int constraint_iters = 4) {
        // Apply gravity and integrate (Verlet).
        for (auto& p : particles_) {
            if (p.pinned) continue;
            p.accel = gravity;
            Vec2 vel = p.pos - p.prev;
            p.prev = p.pos;
            p.pos = p.pos + vel + p.accel * (dt * dt);
        }
        // Satisfy constraints via relaxation.
        for (int i = 0; i < constraint_iters; ++i) {
            for (auto& c : constraints_) {
                auto& pa = particles_[c.a];
                auto& pb = particles_[c.b];
                Vec2 delta = pb.pos - pa.pos;
                double dist = delta.length();
                if (dist < 1e-12) continue;
                double diff = (dist - c.rest_length) / dist * 0.5;
                Vec2 offset = delta * diff;
                if (!pa.pinned) pa.pos = pa.pos + offset;
                if (!pb.pinned) pb.pos = pb.pos - offset;
            }
        }
    }

    /// @brief Read-only access to all particles.
    /// @return Const reference to the particle vector.
    [[nodiscard]] const std::vector<Particle>& particles() const { return particles_; }

private:
    std::vector<Particle> particles_;
    std::vector<DistanceConstraint> constraints_;
};

}  // namespace simulation
