# src/core — core library implementation

Scope: `src/core/`. Backs target **`core` (STATIC)**. Namespace `core`.
Header contract: [include/core/AGENTS.md](../../include/core/AGENTS.md). Parent: [src/AGENTS.md](../AGENTS.md).

## Files
- `app.cpp` — defines `struct App::Impl` (holds the `AppConfig`) and all `App` members whose
  definitions must see a complete `Impl`: constructor, destructor, moves, `run()`, `name()`.

## Why the split (business rationale)
This is the **PImpl idiom**. `app.h` exposes only `std::unique_ptr<Impl> impl_`; the concrete
`Impl` and its heavy dependencies (spdlog) live here. That gives `core` a stable ABI and keeps
compile times low for every consumer. Respect the boundary — it is the whole point of the module.

## Rules specific to this file
1. **Special members that need a complete `Impl` are defined here**, as `= default`:
   `App::~App()`, the move ctor, and move assignment. They are only *declared* in the header.
   Keep them here — moving them into the header breaks compilation (incomplete `Impl`).
2. New per-app state → add a member to `struct App::Impl`, not to the header.
3. **Optional logging must degrade gracefully.** All spdlog use is under `#ifdef HAS_SPDLOG`
   with an `<iostream>` fallback. Keep both branches working — CI builds configurations
   without spdlog. `log_level` is cast to `spdlog::level::level_enum`.
4. `AppConfig` is moved into `Impl` (`std::move`) — it's a sink; don't copy it.

## Best practices
- Include `"core/app.h"` first (the include path root is `include/`).
- Keep this TU warning-clean under the strict flags; no `using namespace` at file scope.
- `run()` returns an exit code — `0` means success; return non-zero for real failures.

## Verify
`cmake --build build/default --target core` then run `ctest --preset default -R core`.
Test source: `tests/core_tests.cpp`.
