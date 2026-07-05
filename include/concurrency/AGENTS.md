# concurrency — thread pool, lock-free SPSC queue, parallel helpers

Scope: `include/concurrency/`. Namespace `concurrency`. CMake target **`concurrency`
(INTERFACE / header-only)**, links `Threads::Threads`.
Parent: [include/AGENTS.md](../AGENTS.md).

## Business role
Reusable threading primitives. This is one of the most **safety-critical** modules: the
`concurrency-*` clang-tidy checks are **errors**, and CI runs the whole suite under
**ThreadSanitizer** (`tsan` preset). Any data race here fails the build.

## Files
- `thread_pool.h` — `ThreadPool`: fixed-size pool of `std::jthread`; `submit(f, args...)`
  returns `std::future`. Cooperative shutdown via `std::stop_token`.
- `lock_free_queue.h` — `SpscQueue<T, Cap>`: single-producer/single-consumer ring buffer,
  no mutex, `std::atomic` with acquire/release ordering.
- `parallel.h` — `parallel_for` and `parallel_map_reduce` free functions built on `std::async`.

## Invariants & business rules (MUST hold — these are correctness, not style)
1. **`SpscQueue` is SPSC, period.** *Exactly one* thread may ever call `push()` and *exactly
   one* (possibly different) thread may call `pop()`. It is **not** MPMC/MPSC-safe. If you
   need multiple producers/consumers, build a different type — do not weaken this one.
2. `SpscQueue` capacity **must be a power of two** (`static_assert` enforces it) — the fast
   path masks with `Cap - 1` instead of `%`. `push` returns `false` when full; `pop` returns
   `std::nullopt` when empty. `head_`/`tail_` are `alignas(64)` to avoid false sharing —
   keep that.
3. **Memory ordering is load-bearing.** The producer publishes with `release`, the consumer
   observes with `acquire`. Do not relax these to `relaxed` or "simplify" them; TSan/readers
   rely on the happens-before edge. Element type must be default-constructible (the buffer is
   value-initialized) and cheaply movable.
4. `ThreadPool` uses `std::jthread` (auto-joins) + `request_stop()` in the destructor. Do not
   detach workers or add a raw `std::thread`. `submit` wraps the call in a
   `std::packaged_task` and returns its `future` — exceptions propagate through `future.get()`.
5. Guard shared state (`tasks_`, the queue) with the existing `mutex`/`condition_variable`;
   `notify_one` on submit, `notify_all` on shutdown.
6. `parallel_for`/`parallel_map_reduce` **capture the user function by reference** and
   propagate exceptions via `future.get()`. The callable must be safe to invoke concurrently
   from multiple threads (no shared mutable state without synchronization).

## C++ best practices for this module
- Prefer C++20 primitives already in use: `std::jthread`, `std::stop_token`, `std::atomic`.
- Never hold a lock across a user callback or a `.get()`.
- Reason about ordering explicitly; annotate why each `memory_order` is what it is.
- Test with **`cmake --preset tsan && ctest --preset tsan`** before finishing any change here.

## When editing
- Update `tests/concurrency_tests.cpp` (tag `[concurrency]`). Throughput lives in
  `benchmarks/benchmarks.cpp` (`BM_SpscThroughput`).
- New primitive → add the header, re-run TSan, and document its threading contract in its
  Doxygen `@brief` (who may call what, from how many threads).

## Neighbors
Higher-level parallel data ops live in [etl](../etl/AGENTS.md) (MapReduce) and
[hpc](../hpc/AGENTS.md) (parallel-STL). Deep dive: [docs/hpc_optimization.md](../../docs/hpc_optimization.md).
