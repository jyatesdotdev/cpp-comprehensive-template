# Hardening TODO

Living tracker. Parent audit + scout/reviewer workflow
(`4c7b8536-ca88-4637-956c-9529f5cd4c8e`) fed this list.

Local verification (2026-09-06, Homebrew Catch2 + system SQLite, no vcpkg, no
cpp-httplib): `cmake -S . -B build/local` → **66/66 tests passed**. `api_tests`
did not build (httplib absent). GitHub Actions were not executed from this
machine.

Status: `[ ]` open · `[x]` done · `[-]` rejected

---

## P0 — Correctness / security

- [x] **P0.1 App log_level mapping** — explicit spdlog conversion in `src/core/app.cpp`
- [x] **P0.2 Arena aligned_alloc / overflow** — round capacity, power-of-two alignment, overflow-safe sizes
- [x] **P0.3 `make_c_buffer` allocation failure** — null check + `<cstdlib>`/`<new>`
- [x] **P0.4 `visitor.h` self-contained** — `#include <memory>`
- [x] **P0.5 ThreadPool lifetime** — drain on stop; `workers_` declared last so joins happen before mutex/cv/queue dtor; moves deleted
- [x] **P0.6 REST errors / timeouts** — generic 500, JSON parse → 400, client timeouts, listen example is `127.0.0.1`
- [x] **P0.7 SQLite transaction / close** — ROLLBACK cannot hide original exception; `sqlite3_close_v2`
- [x] **P0.8 CI vcpkg / bc / Trivy** — bootstrap vcpkg in workspace; install `bc`; pin `trivy-action@0.28.0`; `permissions: contents: read` on CI
- [x] **P0.9 `__restrict` / FMA** — standard pointers; FMA gated on `__FMA__`

---

## P1 — Quality, tests, docs, maintainability

- [x] **P1.1 Missing tests** — UniqueHandle, ThreadPool, parallel_*, map_reduce/flat_map, CRTP/visitor/erasure/ScopedConnection, ECS/physics, hpc, database; `api_tests.cpp` guarded on httplib. Rendering still untested (no GL in CI) — see remaining.
- [x] **P1.2 Documentation overclaims** — README/ARCHITECTURE no longer claim Vulkan module, scene graph, or strategy; LICENSE added; type-erasure SBO claim removed; CloseFd example fixed
- [x] **P1.3 Missing includes** — crtp string_view, numerical utility, rest_server cstdio/exception, output_format `std::size_t`, api_demo chrono/string
- [x] **P1.4 SPSC Cap-1 documented** — Doxygen + `capacity()` + AGENTS.md
- [x] **P1.5 CLI / Sensor robustness** — ProgressBar clamps; Sensor deserialize requires `:`
- [x] **P1.6 CMake packaging (partial)** — CLI11 URL+SHA256; `ENABLE_WERROR`; default preset `RelWithDebInfo`; header `install()`; SQLite target name compatibility
- [x] **P1.7 Physics / ECS / rk4** — index check; pinned constraint correction; ECS `each` snapshots IDs; `dt <= 0` throws
- [x] **P1.9 API demo bind race** — `bind_to_port` + `listen_after_bind`
- [x] **P1.10 Dockerfile** — manifest `vcpkg install` (no `|| true`); removed one-shot HEALTHCHECK

### Still open

- [x] **P1.1c** cpp-httplib FetchContent fallback so `api` / `api_tests` build without vcpkg
- [x] **P1.1d** nlohmann/json FetchContent fallback so `HAS_JSON` works without vcpkg
- [x] **P1.1b** Rendering stays example-only without a display (no GL-context tests; no new CI job)
- [x] **P1.6b** `vcpkg.json` `builtin-baseline` `04a9d8e5212d01ee1dd9478eadd9caade4f8b0d4`
- [x] **P1.6c** CMake package export: `CppComprehensiveTemplateConfig.cmake` + STATIC/INTERFACE
      targets; FetchContent/tooling stay `BUILD_INTERFACE`; `cli` is not exported
- [x] **P1.6d** `ENABLE_WERROR=ON` on `ci`, sanitizer, coverage, and `security-scan` presets
- [x] **P1.8** `compile_shader`/`link_program`/`build_program` return `gl::Shader`/`gl::Program`; `RenderPass` restores FBO and viewport
- [x] **P1.10b** Dockerfile clones the same vcpkg commit (git fetch + checkout), not floating HEAD
- [x] **P1.2b** `docs/best_practices.md` C++23 sections labeled future / not required by this C++20 tree
- [x] **P1.2c** README link text for `docs/api_design.md` is C++ API guidelines, not REST

---

## P2 — Polish (not blocking)

- [x] **P2.1** Dropped unused `algorithm`/`optional` from `pipeline.h`; stopped linking Boost.program_options to `core` (`HAS_BOOST` remains on `cli` only; `HAS_FMT` stays on `core`)
- [x] **P2.2** `Drawable` converting ctor is `explicit`; `Function::operator()` is `const`
- [x] **P2.3** REST middleware: non-200 `res.status` skips the handler (CORS/logger stay 200)
- [x] **P2.4** `Repository` rejects `table_name()` that is not `[A-Za-z_][A-Za-z0-9_]*` (`SqliteError`)
- [x] **P2.5** `SpscQueue::push(T&&)` overload; `push(const T&)` kept
- [x] **P2.6** CI matrix `ubuntu-24.04` / `macos-14` / `windows-2022`; MSan job on Clang
- [x] **P2.7** Documented that `World` uses `typeid` and needs RTTI
- [x] **P2.8** `cli::fmt::colorize` is a no-op when `NO_COLOR` is set; `Table`/`ProgressBar` honor it
- [x] **P2.9** `World::destroy` removes all components (ids are not recycled)
- [x] **P2.10** Visitor `/` by zero throws `std::invalid_argument`

---

## Reviewer extras merged

From quality/readability/architecture/security reviewers (verified in code):

- ThreadPool member destruction order (P0.5)
- `parallel_for` divide-by-zero when `hardware_concurrency()==0` — **fixed**
- ECS iterator invalidation in `each` — **fixed**
- FetchContent unpinned — **fixed** (URL hash)
- Sanitizer/coverage flags not MSVC-gated — **fixed** (FATAL_ERROR on MSVC)
- `trivy-action@master` — **pinned**
- No secret-like filenames found

---

## Remaining

All originally skipped items are implemented. Suggested later:
1. Run GitHub Actions (or local `asan-ubsan` / `tsan` / `msan` presets) once `VCPKG_ROOT` is available.
2. Rendering tests only if an offscreen/headless GL policy is adopted.
3. MSan on CI may false-positive on uninstrumented vcpkg libraries — watch the first run.
