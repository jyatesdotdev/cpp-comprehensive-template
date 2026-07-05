# memory — RAII handles, arena allocator, smart-pointer patterns

Scope: `include/memory/`. Namespace `memory`. CMake target **`memory` (STATIC library)**.
Parent: [include/AGENTS.md](../AGENTS.md) · Implementation: [src/memory/AGENTS.md](../../src/memory/AGENTS.md).

## Business role
The template's **memory-management reference**: how to own raw resources safely (Rule of
Five), how to build a custom allocator, and when to reach for `shared_ptr`/custom deleters.
Consumed by `benchmarks/` (Arena vs `std::allocator`) and `examples/memory_demo.cpp`.

## Files
- `resource_handle.h` — `UniqueHandle<Handle, Deleter, Invalid>`: a move-only RAII wrapper
  for **non-pointer** C handles (fds, `HANDLE`, `GLuint`). `CloseFd` + `UniqueFd` are the
  POSIX example (`CloseFd::operator()` is defined in the `.cpp`).
- `arena_allocator.h` — `Arena` (bump/monotonic allocator) + `ArenaAllocator<T>` (STL-compatible).
- `smart_pointers.h` — factory returning `unique_ptr<Widget>`, `FreeDeleter`/`CUniquePtr` for
  C buffers, and `SharedService : enable_shared_from_this`.

## Invariants & business rules (MUST hold)
1. **`UniqueHandle` is move-only.** Copy is deleted; move transfers via `release()`; the dtor
   calls `Deleter{}(handle_)` only when `handle_ != Invalid`. `Invalid` is the sentinel — a
   default-constructed handle owns nothing. Preserve `noexcept` on all of these.
2. **`Arena` semantics are load-bearing:** allocation only bumps a cursor; **`deallocate()`
   is a deliberate no-op**; memory is reclaimed only by `reset()` or destruction. `reset()`
   does **not** run destructors. `allocate()` throws `std::bad_alloc` when capacity is
   exceeded and aligns the cursor up to the requested alignment. `Arena` is
   **move-constructible but NOT move-assignable** (`operator=(Arena&&) = delete`) and
   non-copyable — do not "fix" this; it is intentional.
3. **`ArenaAllocator` holds a pointer to its `Arena`; the arena must outlive every container
   using it.** This is the #1 misuse. Don't store an `ArenaAllocator` past its arena's scope.
4. `SharedService::get_ref()` calls `shared_from_this()` — it is UB / throws `std::bad_weak_ptr`
   unless the object is already owned by a `shared_ptr`. Never call it on a stack instance.

## C++ best practices for this module
- This is the reference site for the **Rule of Five**: `delete` copy, `noexcept` move, release
  in dtor, guard self-assignment, use `std::exchange`/`std::swap` in moves.
- Prefer `make_unique`/`make_shared`; use `CUniquePtr`/custom deleters only for C-allocated
  memory (`malloc`/`aligned_alloc`).
- Watch alignment: `Arena::allocate` must return storage aligned for the requested type
  (`alignof(T)`); the STL allocator path passes `alignof(T)` already.
- Keep everything usable under ASan/UBSan — this module is a prime sanitizer target.

## When editing
- New handle kind → add a `Deleter` struct + a `using` alias (mirror `CloseFd`/`UniqueFd`);
  if it needs a system call, implement the deleter in `src/memory/resource_handle.cpp` behind
  the right platform guard.
- Update `tests/memory_tests.cpp` (tag `[memory]`) and, for allocators, consider a
  `benchmarks/` entry (see `BM_ArenaAllocator`).

## Neighbors
RAII handle pattern parallels [rendering `gl::Resource`](../rendering/AGENTS.md).
Ownership guidance: [docs/best_practices.md](../../docs/best_practices.md#smart-pointers).
