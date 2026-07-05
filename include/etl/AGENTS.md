# etl — lazy pipeline & parallel MapReduce

Scope: `include/etl/`. Namespace `etl`. CMake target **`etl` (INTERFACE / header-only)**,
links `Threads::Threads`. Parent: [include/AGENTS.md](../AGENTS.md).

## Business role
Composable data-transformation building blocks: a lazy, fluent `Pipeline` (map/filter/
flat_map/take → collect/reduce/for_each/count) and a parallel `map_reduce`. Shown in
`examples/etl_demo.cpp` and benchmarked against a raw loop (`BM_EtlPipeline`).

## Files
- `pipeline.h` — `etl::from(range)` builds a `Pipeline<Source<Range>>`; each combinator
  returns a *new* `Pipeline` with an appended stage type. Stages: `Source`, `Map`, `Filter`,
  `FlatMap`, `Take`. Terminals: `collect`, `reduce`, `for_each`, `count`.
- `map_reduce.h` — `map_reduce(data, identity, map_fn, reduce_fn, n_threads=0)` and
  `parallel_map(data, fn, n_threads=0)`: partition across threads via `std::async`.

## Invariants & business rules (MUST hold)
1. **The pipeline is lazy.** Nothing runs until a terminal op is called; combinators only
   build the stage type. Preserve this — don't eagerly materialize intermediate vectors.
2. **`Source` stores `const Range&` (a reference).** The source container **must outlive** the
   pipeline. Never `etl::from(temporary)` and store the pipeline; never return a `Pipeline`
   that closes over a local range. This is the module's #1 lifetime hazard.
3. **`element_type` / `element_type_t` must stay in lockstep with the stages.** Adding a stage
   type means adding its `element_type<>` specialization, or `collect()` won't deduce its
   output type. `Map`'s element type is `invoke_result_t<Fn, input>`; `FlatMap`'s is the inner
   range's `value_type`.
4. `Take` implements early termination by **throwing an internal `StopIteration`** that the
   `Take` evaluator catches. This is intentional control flow, not an error path — keep the
   `try/catch` local and don't let `StopIteration` escape a `Pipeline`.
5. `map_reduce`/`parallel_map`: the `Range` must expose `size()`, `operator[]`, `empty()`
   (index-addressable). `map_fn` runs concurrently on disjoint chunks — it **must be
   thread-safe / stateless**. `reduce_fn` must be **associative** (partial results merge in
   arbitrary order). `identity` must be a true identity for `reduce_fn`. Empty input returns
   `identity` / an empty vector. `n_threads == 0` means hardware concurrency (min 1).

## C++ best practices for this module
- Template-heavy, header-only: keep everything `inline`/in-header; give combinators clear
  `@tparam` docs. Move functors into stages (`std::move(fn)`); forward elements with
  `std::forward<decltype(v)>(v)` in the evaluators.
- Prefer composing existing stages over adding new ones; if you add one, cover map/filter/
  flat_map/take interactions in tests.
- For `map_reduce`, prefer `std::plus<>{}`-style transparent functors.

## When editing
- Update `tests/etl_tests.cpp` (tag `[etl]`). If you change pipeline codegen, re-check
  `benchmarks/benchmarks.cpp` (`BM_EtlPipeline` vs `BM_RawLoop`).
- New combinator → add the stage struct, its `element_type<>` specialization, a `Pipeline`
  method, and an `evaluate()` overload. Miss one and it won't compile — that's the safety net.

## Neighbors
Thread-pool parallelism: [concurrency](../concurrency/AGENTS.md) (`parallel_map_reduce` is a
sibling of this module's `map_reduce`). Vectorized ops: [hpc](../hpc/AGENTS.md).
`std::ranges` is the standard-library analogue of this pipeline (see [docs/best_practices.md](../../docs/best_practices.md)).
