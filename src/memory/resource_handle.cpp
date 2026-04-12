/// @file resource_handle.cpp
/// @brief Implementation of CloseFd for POSIX file descriptor cleanup.

#include "memory/resource_handle.h"

#ifdef __has_include
#if __has_include(<unistd.h>)
#include <unistd.h>
#define HAS_POSIX_CLOSE
#endif
#endif

namespace memory {

void CloseFd::operator()(int fd) const noexcept {
#ifdef HAS_POSIX_CLOSE
    ::close(fd);
#else
    (void)fd; // No-op on non-POSIX platforms
#endif
}

} // namespace memory
