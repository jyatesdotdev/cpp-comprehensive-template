#pragma once
/// @file observer.h
/// @brief Observer / Signal-Slot pattern using modern C++
///
/// Type-safe, multi-listener signal with automatic disconnection via RAII tokens.

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace patterns {

/// @brief A signal that listeners can subscribe to. Thread-safe.
/// @tparam Args Parameter types emitted to connected slots.
template <typename... Args>
class Signal {
public:
    /// @brief Callable type for slots.
    using SlotFn = std::function<void(Args...)>;

    /// @brief RAII handle — destructor auto-disconnects the slot.
    class Connection {
        friend class Signal;
        std::shared_ptr<bool> alive_;
    public:
        Connection() = default;

        /// @brief Manually disconnect this slot.
        void disconnect() { alive_.reset(); }

        /// @brief Check whether the slot is still connected.
        /// @return true if the slot is live.
        [[nodiscard]] bool connected() const { return alive_ != nullptr; }
    };

    /// @brief Subscribe a callable; returns a Connection whose destruction disconnects.
    /// @param fn The slot function to invoke on emit.
    /// @return A Connection handle for managing the subscription lifetime.
    Connection connect(SlotFn fn) {
        auto sentinel = std::make_shared<bool>(true);
        std::lock_guard lk(mu_);
        slots_.push_back({std::move(fn), sentinel});
        Connection c;
        c.alive_ = sentinel;
        return c;
    }

    /// @brief Emit the signal — invokes all live slots.
    /// @param args Arguments forwarded to each connected slot.
    void emit(Args... args) {
        std::vector<Slot> snapshot;
        {
            std::lock_guard lk(mu_);
            // Prune dead slots while copying live ones
            std::erase_if(slots_, [](const Slot& s) { return s.sentinel.expired(); });
            snapshot = slots_;
        }
        for (auto& s : snapshot) {
            if (auto pin = s.sentinel.lock()) {
                s.fn(args...);
            }
        }
    }

    /// @brief Number of currently connected (live) slots.
    /// @return The count of active subscriptions.
    [[nodiscard]] std::size_t size() const {
        std::lock_guard lk(mu_);
        return static_cast<std::size_t>(
            std::count_if(slots_.begin(), slots_.end(),
                          [](const Slot& s) { return !s.sentinel.expired(); }));
    }

private:
    struct Slot {
        SlotFn fn;
        std::weak_ptr<bool> sentinel;
    };
    mutable std::mutex mu_;
    std::vector<Slot> slots_;
};

/// @brief Convenience RAII wrapper that disconnects a Signal::Connection on destruction.
///
/// The constructor is templated on the connection type (deduced from the argument), because
/// a signal's `Args...` cannot be deduced from a nested `Signal<Args...>::Connection` — that
/// is a non-deduced context. The `requires` clause restricts it to connection-like types.
class ScopedConnection {
    std::function<void()> disconnect_;
public:
    ScopedConnection() = default;

    /// @brief Take ownership of a connection; its slot is disconnected when this object dies.
    /// @tparam Conn A `Signal<...>::Connection` type (any signal signature).
    /// @param c The connection to manage.
    template <typename Conn>
        requires requires(Conn c) { c.disconnect(); }
    explicit ScopedConnection(Conn c)
        : disconnect_([c]() mutable { c.disconnect(); }) {}

    ~ScopedConnection() { if (disconnect_) disconnect_(); }

    ScopedConnection(ScopedConnection&&) noexcept = default;
    ScopedConnection& operator=(ScopedConnection&&) noexcept = default;
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
};

}  // namespace patterns
