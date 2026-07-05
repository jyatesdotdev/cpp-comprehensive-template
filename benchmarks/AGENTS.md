# benchmarks/ — Google Benchmark micro-benchmarks

Scope: `benchmarks/`. Parent: [root AGENTS.md](../AGENTS.md). Framework: **Google Benchmark** (vcpkg).

## Purpose & business rules
Micro-benchmarks that justify the template's performance claims by comparing an abstraction
against a naive baseline. `benchmarks.cpp` currently covers:
- `BM_ArenaAllocator` vs `BM_StdAllocator` — custom [memory](../include/memory/AGENTS.md) arena.
- `BM_EtlPipeline` vs `BM_RawLoop` — lazy [etl](../include/etl/AGENTS.md) pipeline overhead.
- `BM_SpscThroughput` — [concurrency](../include/concurrency/AGENTS.md) SPSC queue throughput.

The target links `memory etl concurrency` and only builds when **`benchmark_FOUND`** *and*
`BUILD_BENCHMARKS=ON` (which is **OFF by default** — enable it explicitly).

## Writing a benchmark
- Pattern: `static void BM_X(benchmark::State& state) { for (auto _ : state) { …work… } }`
  then `BENCHMARK(BM_X)->Range(lo, hi);`. `benchmark_main` provides `main()` — don't add one.
- **Defeat the optimizer** or the loop gets deleted: wrap results in
  `benchmark::DoNotOptimize(...)` (and `benchmark::ClobberMemory()` when needed). Every existing
  benchmark does this — keep it.
- Do fixed setup **outside** the `for (auto _ : state)` loop; only the measured work goes inside.
- Use `state.range(0)` for sized inputs and `state.SetItemsProcessed(...)` for throughput.
- Compare against a **baseline** — a benchmark of an abstraction is only meaningful next to the
  naive version it claims to beat (or match). Keep the pair.

## Adding a new benchmark target's dependency
If you benchmark another module, add its target to `target_link_libraries(benchmarks PRIVATE …)`.
Header-only modules just need their include path (already provided by linking the target).

## Build & run
```bash
cmake --preset release -DBUILD_BENCHMARKS=ON && cmake --build build/release --target benchmarks
./build/release/benchmarks                       # all
./build/release/benchmarks --benchmark_filter=Arena
```
**Benchmark in a `release` build** — numbers from debug/sanitizer builds are meaningless.
"Faster" claims must be measured, not asserted. Deeper guidance:
[docs/hpc_optimization.md](../docs/hpc_optimization.md).

## Conventions
C++20, 4-space/100-col, warning-clean. See [root AGENTS.md](../AGENTS.md).
