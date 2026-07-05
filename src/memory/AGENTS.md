# src/memory — memory library implementation

Scope: `src/memory/`. Backs target **`memory` (STATIC)**. Namespace `memory`.
Header contract: [include/memory/AGENTS.md](../../include/memory/AGENTS.md). Parent: [src/AGENTS.md](../AGENTS.md).

## Files
- `resource_handle.cpp` — the **only** out-of-line symbol in `memory`: `CloseFd::operator()`,
  which closes a POSIX file descriptor. Everything else in the module is header-only
  (templates: `UniqueHandle`, `Arena`, `ArenaAllocator`).

## Rules specific to this file
1. **Platform-guard the syscall.** `::close(fd)` is only compiled when `<unistd.h>` is
   available (detected via `__has_include`, which defines `HAS_POSIX_CLOSE`). On non-POSIX
   platforms it is a `(void)fd;` no-op. Keep both paths — the library must compile everywhere.
2. `CloseFd::operator()` is `noexcept` (a deleter must never throw). Preserve that.
3. Only add a `.cpp` symbol here when it *cannot* be header-only (needs a system header you
   don't want leaking into the public header, or must be a single definition). Prefer keeping
   new allocator/handle logic header-only in `include/memory/`.

## Best practices
- Include `"memory/resource_handle.h"` first.
- Deleters invoked by RAII destructors must be `noexcept` and tolerant of being called during
  stack unwinding.

## Verify
`ctest --preset default -R memory`. Test source: `tests/memory_tests.cpp`.
This module is a key **ASan/UBSan** target — validate with `cmake --preset asan-ubsan` when
touching allocation or handle lifetime.
