#pragma once

/// @file arena_allocator.h
/// @brief A simple arena (bump) allocator with STL container support.
///
/// The arena pre-allocates a contiguous block and hands out memory via pointer
/// bumping. Individual deallocations are no-ops; the entire arena is freed at
/// once when the Arena object is destroyed (or explicitly reset).

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>

namespace memory {

/// @brief Owns a contiguous memory block. Allocations bump a cursor forward;
/// individual frees are no-ops. Reset or destruction reclaims everything.
class Arena {
public:
    /// @brief Construct an arena with the given byte capacity.
    /// @param capacity Size in bytes of the pre-allocated block.
    /// @throws std::bad_alloc if the allocation fails.
    explicit Arena(std::size_t capacity)
        : capacity_{capacity},
          buf_{static_cast<std::byte*>(std::aligned_alloc(alignof(std::max_align_t), capacity))} {
        if (!buf_) throw std::bad_alloc{};
    }

    ~Arena() { std::free(buf_); }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& o) noexcept : capacity_{o.capacity_}, offset_{o.offset_}, buf_{o.buf_} {
        o.buf_ = nullptr;
        o.offset_ = 0;
    }
    Arena& operator=(Arena&&) = delete;

    /// @brief Allocate @p size bytes with @p alignment.
    /// @param size Number of bytes to allocate.
    /// @param alignment Required alignment (defaults to max fundamental alignment).
    /// @return Pointer to the allocated memory.
    /// @throws std::bad_alloc if the arena has insufficient remaining capacity.
    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
        auto aligned = (offset_ + alignment - 1) & ~(alignment - 1);
        if (aligned + size > capacity_) throw std::bad_alloc{};
        offset_ = aligned + size;
        return buf_ + aligned;
    }

    /// @brief No-op — arena frees everything on destruction or reset().
    /// @param ptr Unused.
    /// @param size Unused.
    void deallocate(void* /*ptr*/, std::size_t /*size*/) noexcept {}

    /// @brief Reclaim all memory (does not call destructors).
    void reset() noexcept { offset_ = 0; }

    /// @brief Return the number of bytes currently allocated.
    [[nodiscard]] std::size_t used() const noexcept { return offset_; }
    /// @brief Return the total capacity in bytes.
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

private:
    std::size_t capacity_;
    std::size_t offset_{0};
    std::byte* buf_;
};

/// @brief STL-compatible allocator backed by an Arena.
/// @tparam T The value type to allocate.
/// @note The Arena must outlive all containers using this allocator.
template <typename T>
class ArenaAllocator {
public:
    using value_type = T;

    /// @brief Construct from an existing Arena.
    /// @param arena The backing arena (must outlive this allocator).
    explicit ArenaAllocator(Arena& arena) noexcept : arena_{&arena} {}

    /// @brief Converting constructor for rebinding.
    template <typename U>
    ArenaAllocator(const ArenaAllocator<U>& o) noexcept : arena_{o.arena_} {}

    /// @brief Allocate storage for @p n objects of type T.
    /// @param n Number of objects.
    /// @return Pointer to uninitialized storage.
    /// @throws std::bad_alloc if the arena has insufficient capacity.
    T* allocate(std::size_t n) {
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    /// @brief Deallocate storage (no-op for arena allocator).
    /// @param p Pointer to storage.
    /// @param n Number of objects.
    void deallocate(T* p, std::size_t n) noexcept {
        arena_->deallocate(p, n * sizeof(T));
    }

    template <typename U>
    bool operator==(const ArenaAllocator<U>& o) const noexcept { return arena_ == o.arena_; }

private:
    template <typename U> friend class ArenaAllocator;
    Arena* arena_;
};

} // namespace memory
