#pragma once
/// @file numerical.h
/// @brief Numerical computing utilities — RK4 ODE solver and Simpson's rule.

#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

namespace simulation {

/// @brief Fourth-order Runge-Kutta integrator for dy/dt = f(t, y).
/// @tparam F Callable with signature `double(double t, double y)`.
/// @param f Right-hand side function f(t, y).
/// @param y0 Initial value of y at @p t0.
/// @param t0 Start time.
/// @param t_end End time.
/// @param dt Time step size.
/// @return Vector of (t, y) pairs from @p t0 to @p t_end.
template <typename F>
auto rk4(F&& f, double y0, double t0, double t_end, double dt)
    -> std::vector<std::pair<double, double>> {
    std::vector<std::pair<double, double>> result;
    double t = t0, y = y0;
    result.emplace_back(t, y);
    while (t < t_end - dt * 0.5) {
        double k1 = dt * f(t, y);
        double k2 = dt * f(t + dt / 2, y + k1 / 2);
        double k3 = dt * f(t + dt / 2, y + k2 / 2);
        double k4 = dt * f(t + dt, y + k3);
        y += (k1 + 2 * k2 + 2 * k3 + k4) / 6;
        t += dt;
        result.emplace_back(t, y);
    }
    return result;
}

/// @brief Simpson's rule for definite integral of f over [a, b].
/// @tparam F Callable with signature `double(double)`.
/// @param f Integrand function.
/// @param a Lower bound of integration.
/// @param b Upper bound of integration.
/// @param n Number of subdivisions (rounded up to even if odd).
/// @return Approximate value of the integral.
/// @pre @p n should be even for correct results; if odd it is incremented.
template <typename F>
double simpson(F&& f, double a, double b, std::size_t n = 1000) {
    if (n % 2 != 0) ++n;
    double h = (b - a) / static_cast<double>(n);
    double sum = f(a) + f(b);
    for (std::size_t i = 1; i < n; ++i)
        sum += f(a + static_cast<double>(i) * h) * (i % 2 == 0 ? 2.0 : 4.0);
    return sum * h / 3.0;
}

}  // namespace simulation
