# tests/ — Catch2 unit tests

Scope: `tests/`. Parent: [root AGENTS.md](../AGENTS.md). Framework: **Catch2 v3** (vcpkg).

## Layout & business rules
- **One test file per module**, named `<module>_tests.cpp`. Current targets (from
  `CMakeLists.txt`): `core`, `memory`, `concurrency`, `etl`, `patterns`, `simulation`, `cli`.
  (`hpc`, `api`, `database`, `rendering` have **no** test target yet — the first three because
  no one added one, `rendering` because CI has no GL context. Adding tests for these is welcome.)
- Tests only build when **`Catch2_FOUND`** (the whole dir `return()`s early otherwise) and when
  `BUILD_TESTS=ON` (default).
- Registration is one line via the local helper:
  ```cmake
  add_catch_test(<name> <libs...>)   # links Catch2::Catch2WithMain + your module target(s)
  ```
  It calls `catch_discover_tests`, so each `TEST_CASE` shows up as its own CTest test.

## Conventions for writing a test
- Include `<catch2/catch_test_macros.hpp>` (and matchers/etc. as needed), then the module header.
- Use `TEST_CASE("clear description", "[module][sub]")` — the **first tag is the module name**
  (`[core]`, `[patterns][observer]`). Tags are how suites are filtered.
- Prefer `REQUIRE` (fatal) for invariants, `CHECK` (non-fatal) for independent sub-assertions,
  `REQUIRE_NOTHROW`/`REQUIRE_THROWS_AS` for exception contracts, `SECTION`s for shared setup.
- Test the **documented contract and the business rules** in each module's `AGENTS.md`:
  e.g. observer auto-disconnect, arena no-op free, SPSC full/empty, SQLite bind indexing.
- Keep tests deterministic and independent (no shared global state, no ordering assumptions —
  discovery may run them in any order/parallel).

## The coverage gate (do not ignore)
CI runs the `coverage` preset and **fails the build under 80% line coverage** (see
[.github/workflows/AGENTS.md](../.github/workflows/AGENTS.md)). New non-trivial code needs
tests, or the module's coverage can drop the whole project below threshold. `tests/` itself is
excluded from the coverage denominator.

## Run
```bash
cmake --preset default && cmake --build build/default
ctest --preset default --output-on-failure          # all tests
ctest --preset default -R core                       # one module by name/regex
cmake --preset coverage && cmake --build build/coverage && ctest --preset coverage  # coverage
```
Run the **`asan-ubsan`** and **`tsan`** presets for memory/concurrency changes — CI does, and
sanitizer failures there block merges.

## Conventions
C++20, 4-space / 100-col, warning-clean. See [root AGENTS.md](../AGENTS.md).
