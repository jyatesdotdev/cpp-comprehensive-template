#pragma once
/// @file crtp.h
/// @brief CRTP (Curiously Recurring Template Pattern) — static polymorphism
///
/// Eliminates virtual dispatch overhead while enforcing interfaces at compile time.

#include <concepts>
#include <cstddef>
#include <iostream>
#include <string>

namespace patterns {

// ── Base CRTP helper with self() accessor ───────────────────────────

/// @brief Base CRTP helper providing a `self()` accessor that casts to the Derived type.
/// @tparam Derived The concrete class inheriting from this base.
template <typename Derived>
struct Crtp {
    /// @brief Access the derived object (mutable).
    /// @return Reference to the derived instance.
    [[nodiscard]] constexpr auto& self() noexcept {
        return static_cast<Derived&>(*this);
    }
    /// @brief Access the derived object (const).
    /// @return Const reference to the derived instance.
    [[nodiscard]] constexpr const auto& self() const noexcept {
        return static_cast<const Derived&>(*this);
    }
};

// ── Static interface: Serializable ──────────────────────────────────

/// @brief CRTP mixin that adds `serialize()` / `deserialize()` by delegating to Derived.
/// @tparam Derived Must implement `do_serialize()` and `do_deserialize(std::string_view)`.
template <typename Derived>
struct Serializable : Crtp<Derived> {
    /// @brief Serialize the object to a string.
    /// @return The serialized representation.
    [[nodiscard]] std::string serialize() const {
        return this->self().do_serialize();
    }
    /// @brief Deserialize from a string, populating the object's state.
    /// @param data The serialized data.
    void deserialize(std::string_view data) {
        this->self().do_deserialize(data);
    }
};

// ── Mixin: Printable (chainable CRTP layer) ─────────────────────────

/// @brief CRTP mixin that adds a `print()` method by delegating to `Derived::to_string()`.
/// @tparam Derived Must implement `std::string to_string() const`.
template <typename Derived>
struct Printable : Crtp<Derived> {
    /// @brief Print the object's string representation to an output stream.
    /// @param os The output stream (defaults to std::cout).
    void print(std::ostream& os = std::cout) const {
        os << this->self().to_string() << '\n';
    }
};

// ── Mixin: Comparable (operator<=> via CRTP) ────────────────────────

/// @brief CRTP mixin that provides `==` and `<=>` operators via `Derived::compare_key()`.
/// @tparam Derived Must implement a `compare_key() const` returning a three-way-comparable value.
template <typename Derived>
struct Comparable : Crtp<Derived> {
    [[nodiscard]] friend bool operator==(const Derived& a, const Derived& b) {
        return a.compare_key() == b.compare_key();
    }
    [[nodiscard]] friend auto operator<=>(const Derived& a, const Derived& b) {
        return a.compare_key() <=> b.compare_key();
    }
};

// ── Example concrete type using multiple CRTP mixins ────────────────

/// @brief Example sensor type demonstrating Serializable, Printable, and Comparable mixins.
struct Sensor : Serializable<Sensor>,
                Printable<Sensor>,
                Comparable<Sensor> {
    std::string name;  ///< Sensor identifier.
    double value{};    ///< Current reading.

    /// @brief Serialize to "name:value" format.
    /// @return The serialized string.
    [[nodiscard]] std::string do_serialize() const {
        return name + ":" + std::to_string(value);
    }
    /// @brief Deserialize from "name:value" format.
    /// @param data The serialized string.
    void do_deserialize(std::string_view data) {
        auto sep = data.find(':');
        name = std::string(data.substr(0, sep));
        value = std::stod(std::string(data.substr(sep + 1)));
    }
    /// @brief Human-readable representation.
    /// @return String of the form "Sensor(name, value)".
    [[nodiscard]] std::string to_string() const {
        return "Sensor(" + name + ", " + std::to_string(value) + ")";
    }
    /// @brief Key used for comparison (orders sensors by value).
    /// @return The sensor's numeric value.
    [[nodiscard]] double compare_key() const { return value; }
};

}  // namespace patterns
