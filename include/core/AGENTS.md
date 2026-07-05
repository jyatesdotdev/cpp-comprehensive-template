# core — Application lifecycle & the PImpl idiom

Scope: `include/core/`. Namespace `core`. CMake target **`core` (STATIC library)**.
Parent: [include/AGENTS.md](../AGENTS.md) · Implementation: [src/core/AGENTS.md](../../src/core/AGENTS.md).

## Business role
`core` is the **foundation module** and the template's reference example of a stable,
ABI-safe library boundary. `hello_world` and `third_party_demo` link it; it is the first
thing a new user builds. It is the canonical demonstration of **RAII lifecycle + PImpl** in
this repo — patterns the rest of the codebase is expected to follow.

## Files
- `app.h` — `AppConfig` (name + log level) and `App`, a move-only application object built
  on the **PImpl idiom** (`struct Impl; std::unique_ptr<Impl> impl_;`).

## Key API & contracts
- `AppConfig{ std::string name = "CppTemplate"; int log_level = 1; }` — `log_level`:
  0=trace, 1=info, 2=warn, 3=error (maps to spdlog levels in the impl).
- `explicit App(AppConfig = {})`, `int run()` (returns exit code, 0 = success),
  `std::string_view name() const noexcept`.
- **Move-only**: copy is `= delete`d; move ctor/assign are `noexcept`.

## Invariants & business rules (MUST hold)
1. **Preserve the PImpl boundary.** Keep `App`'s data as a single `std::unique_ptr<Impl>`.
   Never expose implementation types, heavy includes (spdlog, etc.), or data members in
   `app.h` — the whole point is a stable ABI and fast compiles. New implementation state
   goes inside `struct App::Impl` in `src/core/app.cpp`, not the header.
2. Because `Impl` is incomplete in the header, the **destructor and moves must be declared
   here and defined in the `.cpp`** (they are `= default` there, where `Impl` is complete).
   Do not `= default` them in the header — it won't compile.
3. Keep `App` **move-only** (an app owns process-lifetime resources; copying is meaningless).
4. `name()` returns a view into `Impl`-owned storage — keep that storage alive for the
   object's lifetime (it currently is; don't return views to temporaries).

## C++ best practices for this file
- `explicit` single-arg constructors (already applied) to avoid surprising conversions.
- `[[nodiscard]]` + `noexcept` on `name()`; `noexcept` on the moves.
- Prefer designated initializers for config at call sites: `App({.name = "X", .log_level = 3})`.
- Take `AppConfig` **by value** and `std::move` it into `Impl` (sink parameter — already done).

## When editing
- Adding a config field → add to `AppConfig` **and** thread it through `App::Impl` in the `.cpp`.
- Update `tests/core_tests.cpp` (Catch2, tag `[core]`) and, if user-visible, the demos.
- Logging is optional: real logic must compile with `HAS_SPDLOG` **undefined** (the `.cpp`
  has an `iostream` fallback — preserve it).

## Neighbors
Same RAII/move discipline appears in [memory](../memory/AGENTS.md) (handles) and
[rendering](../rendering/AGENTS.md) (GL objects). PImpl rationale: [docs/best_practices.md](../../docs/best_practices.md).
