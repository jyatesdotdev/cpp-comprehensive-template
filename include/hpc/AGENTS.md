# hpc — SIMD intrinsics & parallel-STL wrappers

Scope: `include/hpc/`. Namespace `hpc`. CMake target **`hpc` (INTERFACE / header-only)**.
Parent: [include/AGENTS.md](../AGENTS.md).

## Business role
The performance-primitives module: hand-vectorized float kernels plus thin wrappers over
C++17 parallel algorithms. Demonstrated in `examples/hpc_demo.cpp`; the SIMD-vs-scalar
contrast is a teaching goal. See [docs/hpc_optimization.md](../../docs/hpc_optimization.md).

## Files
- `simd_ops.h` — `simd_add`, `simd_mul`, `simd_dot` (+ `scalar_dot` reference). Auto-detects
  **AVX2 / SSE2 / NEON** at compile time; `kSimdWidth` is the lane count (8 / 4 / 1).
- `parallel_stl.h` — `par_sort`, `par_transform`, `par_reduce`, `par_transform_reduce`,
  `par_for_each`: forward to `std::execution::par_unseq` when the parallel algorithms are
  actually available (feature-test macros `__cpp_lib_execution` +
  `__cpp_lib_parallel_algorithm` — **not** mere `<execution>` header presence; Apple libc++
  ships the header without the policies), else fall back to the sequential algorithm.

## Invariants & business rules (MUST hold)
1. **Every SIMD kernel needs a correct scalar tail** and a scalar fallback path. The vector
   loop handles `i + width <= n`; the trailing `for (; i < n; ++i)` handles the remainder.
   Never assume `n` is a multiple of the lane width. Results must match `scalar_dot`/the naive
   loop bit-for-bit-close (floating-point associativity aside — see #3).
2. **Preserve the platform `#if` ladder.** Order is `HPC_SIMD_AVX2` → `HPC_SIMD_SSE2` →
   `HPC_SIMD_NEON` → scalar. Add new ISAs as new branches; keep the scalar `#else` so the code
   compiles on any target. `kSimdWidth` must stay consistent with the active branch.
3. **Reductions are not bit-exact across paths** (SIMD sums in a different order). That is
   expected; tests/benchmarks compare with tolerance. Don't "fix" it by forcing scalar order.
4. Use **unaligned** loads/stores (`_mm256_loadu_ps`, `vld1q_f32`) — callers are not required
   to align their arrays. Keep `__restrict` on the non-aliasing pointer params.
5. `par_*` wrappers must keep the `#if HPC_HAS_PARALLEL_STL` fallback compiling and returning
   the same result sequentially. `par_unseq` forbids inter-element dependencies and
   synchronization in the callable — document that on any new wrapper.

## C++ best practices for this module
- Keep intrinsics isolated behind these small functions; callers stay portable.
- Prefer `std::size_t` for indices/counts; cast intentionally (the strict build flags
  `-Wconversion -Wsign-conversion` will flag sloppy casts).
- FMA (`_mm256_fmadd_ps`) is used on the AVX2 path — it is available in practice with AVX2 but
  note the assumption if you target odd toolchains.
- Benchmark real changes; "faster" must be measured (Google Benchmark, `release` preset).

## When editing
- There is currently **no `hpc_tests.cpp`** and no hpc test target — if you add non-trivial
  logic, add one and wire it into `tests/CMakeLists.txt` (`add_catch_test(hpc_tests hpc)`).
- Update `examples/hpc_demo.cpp` and the optimization doc when behavior/perf characteristics change.

## Neighbors
Thread-level parallelism: [concurrency](../concurrency/AGENTS.md).
Data-flow parallelism: [etl](../etl/AGENTS.md).
