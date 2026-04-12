#pragma once
/// @file type_erasure.h
/// @brief Type erasure — runtime polymorphism without inheritance hierarchies
///
/// Uses a small-buffer-optimized value type that can hold any callable/drawable/etc.
/// Avoids forcing users to inherit from an abstract base class.

#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace patterns {

/// @brief A type-erased "Drawable" — any type with `std::string draw() const` can be stored.
///
/// Uses the external-polymorphism / type-erasure idiom to avoid inheritance hierarchies.
class Drawable {
    // ── Concept (internal interface) ────────────────────────────────
    struct Concept {
        virtual ~Concept() = default;
        [[nodiscard]] virtual std::string draw() const = 0;
        [[nodiscard]] virtual std::unique_ptr<Concept> clone() const = 0;
    };

    template <typename T>
    struct Model final : Concept {
        T obj;
        explicit Model(T o) : obj(std::move(o)) {}
        [[nodiscard]] std::string draw() const override { return obj.draw(); }
        [[nodiscard]] std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(obj);
        }
    };

    std::unique_ptr<Concept> impl_;

public:
    /// @brief Construct from any type that provides `std::string draw() const`.
    /// @tparam T The concrete drawable type.
    /// @param obj The object to wrap.
    template <typename T>
    Drawable(T obj) : impl_(std::make_unique<Model<T>>(std::move(obj))) {}

    Drawable(const Drawable& o) : impl_(o.impl_ ? o.impl_->clone() : nullptr) {}
    Drawable& operator=(const Drawable& o) {
        if (this != &o) impl_ = o.impl_ ? o.impl_->clone() : nullptr;
        return *this;
    }
    Drawable(Drawable&&) noexcept = default;
    Drawable& operator=(Drawable&&) noexcept = default;

    /// @brief Invoke the wrapped object's `draw()` method.
    /// @return The string produced by the underlying object.
    /// @throws std::runtime_error If the Drawable is empty (moved-from).
    [[nodiscard]] std::string draw() const {
        if (!impl_) throw std::runtime_error("empty Drawable");
        return impl_->draw();
    }
};

/// @brief Generic type-erased function object (simplified std::function).
/// @tparam Signature The function signature, e.g. `int(double, double)`.
template <typename Signature>
class Function;

template <typename R, typename... Args>
class Function<R(Args...)> {
    struct Concept {
        virtual ~Concept() = default;
        virtual R invoke(Args...) = 0;
        [[nodiscard]] virtual std::unique_ptr<Concept> clone() const = 0;
    };

    template <typename F>
    struct Model final : Concept {
        F fn;
        explicit Model(F f) : fn(std::move(f)) {}
        R invoke(Args... args) override { return fn(std::forward<Args>(args)...); }
        [[nodiscard]] std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(fn);
        }
    };

    std::unique_ptr<Concept> impl_;

public:
    Function() = default;

    /// @brief Construct from any callable matching the signature.
    /// @tparam F The callable type.
    /// @param f The callable to wrap.
    template <typename F>
    Function(F f) : impl_(std::make_unique<Model<F>>(std::move(f))) {}

    Function(const Function& o) : impl_(o.impl_ ? o.impl_->clone() : nullptr) {}
    Function& operator=(const Function& o) {
        if (this != &o) impl_ = o.impl_ ? o.impl_->clone() : nullptr;
        return *this;
    }
    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    /// @brief Invoke the wrapped callable.
    /// @param args Arguments forwarded to the callable.
    /// @return The result of the callable.
    /// @throws std::runtime_error If the Function is empty.
    R operator()(Args... args) {
        if (!impl_) throw std::runtime_error("empty Function");
        return impl_->invoke(std::forward<Args>(args)...);
    }

    /// @brief Check whether the Function holds a callable.
    /// @return true if non-empty.
    explicit operator bool() const noexcept { return impl_ != nullptr; }
};

}  // namespace patterns
