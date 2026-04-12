#pragma once

/// @file lock_free_queue.h
/// @brief Lock-free single-producer / single-consumer (SPSC) ring buffer.
///
/// Uses std::atomic with acquire/release ordering for safe cross-thread
/// communication without mutexes.

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

namespace concurrency {

/// Fixed-capacity SPSC queue. Exactly one thread may call push(), and exactly
/// one (possibly different) thread may call pop(). No locks are used.
///
/// @tparam T     Element type (must be move-constructible).
/// @tparam Cap   Capacity (must be power of two for efficient masking).
template <typename T, std::size_t Cap = 1024>
class SpscQueue {
    static_assert((Cap & (Cap - 1)) == 0, "Capacity must be a power of two");

public:
    /// @brief Try to enqueue an element.
    /// @param val The value to enqueue (copied into the buffer).
    /// @return `true` if the element was enqueued, `false` if the queue is full.
    bool push(const T& val) {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = (head + 1) & kMask;
        if (next == tail_.load(std::memory_order_acquire)) return false; // full
        buf_[head] = val;
        head_.store(next, std::memory_order_release);
        return true;
    }

    /// @brief Try to dequeue an element.
    /// @return The dequeued value, or `std::nullopt` if the queue is empty.
    std::optional<T> pop() {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return std::nullopt; // empty
        T val = std::move(buf_[tail]);
        tail_.store((tail + 1) & kMask, std::memory_order_release);
        return val;
    }

    /// @brief Check whether the queue is empty.
    /// @return `true` if the queue contains no elements.
    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr std::size_t kMask = Cap - 1;
    std::array<T, Cap> buf_{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

} // namespace concurrency
