#pragma once

/// @file resource_handle.h
/// @brief Generic RAII wrapper for C-style handles (file descriptors, library
///        handles, GPU resources, etc.).
///
/// Usage:
///   auto fd = memory::UniqueHandle<int, CloseFile, -1>(::open(path, O_RDONLY));

#include <utility>

namespace memory {

/// A move-only RAII wrapper around a non-pointer handle type.
/// @tparam Handle   The handle type (int, HANDLE, GLuint, …).
/// @tparam Deleter  Callable that releases the handle.
/// @tparam Invalid  Sentinel value representing "no resource".
template <typename Handle, typename Deleter, Handle Invalid = Handle{}>
class UniqueHandle {
public:
    /// @brief Construct a handle wrapper with no resource.
    UniqueHandle() noexcept = default;
    /// @brief Construct a handle wrapper taking ownership of @p h.
    /// @param h The raw handle to manage.
    explicit UniqueHandle(Handle h) noexcept : handle_{h} {}
    ~UniqueHandle() { reset(); }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& o) noexcept : handle_{o.release()} {}
    UniqueHandle& operator=(UniqueHandle&& o) noexcept {
        if (this != &o) reset(o.release());
        return *this;
    }

    /// @brief Get the underlying handle.
    /// @return The raw handle value.
    [[nodiscard]] Handle get() const noexcept { return handle_; }
    /// @brief Test whether this wrapper holds a valid resource.
    [[nodiscard]] explicit operator bool() const noexcept { return handle_ != Invalid; }

    /// @brief Release ownership and return the raw handle.
    /// @return The previously-owned handle (caller assumes ownership).
    Handle release() noexcept { return std::exchange(handle_, Invalid); }

    /// @brief Replace the managed handle, releasing the old one if valid.
    /// @param h New handle to manage (defaults to Invalid).
    void reset(Handle h = Invalid) noexcept {
        if (handle_ != Invalid) Deleter{}(handle_);
        handle_ = h;
    }

private:
    Handle handle_{Invalid};
};

/// @brief Example deleter for POSIX file descriptors.
struct CloseFd {
    /// @brief Close the file descriptor.
    /// @param fd The file descriptor to close.
    void operator()(int fd) const noexcept;
};

/// @brief Convenience alias for POSIX file descriptors.
using UniqueFd = UniqueHandle<int, CloseFd, -1>;

} // namespace memory
