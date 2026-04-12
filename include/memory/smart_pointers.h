#pragma once

/// @file smart_pointers.h
/// @brief Smart pointer patterns: factories, custom deleters, shared ownership.

#include <memory>
#include <type_traits>

namespace memory {

// ── Factory pattern with unique_ptr ─────────────────────────────────

/// @brief Polymorphic base for demonstrating ownership transfer.
class Widget {
public:
    virtual ~Widget() = default;
    /// @brief Return the widget's value.
    /// @return The integer value held by this widget.
    [[nodiscard]] virtual int value() const noexcept = 0;
};

/// @brief Concrete widget holding an integer.
class IntWidget final : public Widget {
public:
    /// @brief Construct with the given value.
    /// @param v The integer value to store.
    explicit IntWidget(int v) noexcept : v_{v} {}
    /// @copydoc Widget::value()
    [[nodiscard]] int value() const noexcept override { return v_; }
private:
    int v_;
};

/// @brief Factory — returns ownership via unique_ptr (preferred over raw new).
/// @param v The integer value for the widget.
/// @return A unique_ptr owning the newly created IntWidget.
inline std::unique_ptr<Widget> make_widget(int v) {
    return std::make_unique<IntWidget>(v);
}

// ── Custom deleter example ──────────────────────────────────────────

/// @brief Wraps a C-allocated buffer in a unique_ptr with a custom deleter.
struct FreeDeleter {
    /// @brief Free the memory via std::free.
    /// @param p Pointer to free.
    void operator()(void* p) const noexcept { std::free(p); }
};

/// @brief Alias for a unique_ptr using FreeDeleter for C-allocated memory.
template <typename T>
using CUniquePtr = std::unique_ptr<T, FreeDeleter>;

/// @brief Allocate a C-style buffer managed by unique_ptr.
/// @param n Number of bytes to allocate.
/// @return A CUniquePtr owning the allocated buffer.
inline CUniquePtr<char[]> make_c_buffer(std::size_t n) {
    return CUniquePtr<char[]>{static_cast<char*>(std::malloc(n))};
}

// ── enable_shared_from_this example ─────────────────────────────────

/// @brief A shared resource that can hand out shared_ptr to itself.
class SharedService : public std::enable_shared_from_this<SharedService> {
public:
    /// @brief Returns a shared_ptr to this instance.
    /// @return A shared_ptr sharing ownership of this object.
    /// @throws std::bad_weak_ptr if this object is not managed by a shared_ptr.
    std::shared_ptr<SharedService> get_ref() { return shared_from_this(); }
    int id{0}; ///< @brief Identifier for this service instance.
};

} // namespace memory
