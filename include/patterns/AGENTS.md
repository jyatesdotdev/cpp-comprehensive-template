# patterns — modern C++ design patterns

Scope: `include/patterns/`. Namespace `patterns`. CMake target **`patterns` (INTERFACE /
header-only)**, no external deps. Parent: [include/AGENTS.md](../AGENTS.md).

## Business role
A curated catalogue of **idiomatic C++ realizations of classic patterns** — the reference
other modules point to. Each header is meant to be exemplary and self-contained. Shown in
`examples/patterns_demo.cpp`; catalogued in [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).

## Files
- `observer.h` — `Signal<Args...>` (thread-safe signal/slot) + RAII `Connection` /
  `ScopedConnection` that auto-disconnect.
- `type_erasure.h` — `Drawable` and `Function<R(Args...)>`: runtime polymorphism **without**
  a public inheritance hierarchy (internal `Concept`/`Model` + `clone()`).
- `crtp.h` — `Crtp<Derived>` base plus `Serializable`/`Printable`/`Comparable` mixins;
  `Sensor` is the worked example. **Static** polymorphism (no vtables).
- `visitor.h` — `std::variant` + `Overload{}` + `std::visit`; an expression-tree `evaluate`/
  `to_string` over a closed `Expr` set.

## Invariants & business rules (MUST hold)
1. **Observer lifetime is RAII.** A slot stays connected while its `Connection`'s
   `shared_ptr<bool>` sentinel is alive; dropping the token (or `ScopedConnection` going out of
   scope) disconnects. `Signal::emit` snapshots live slots under the mutex, prunes expired
   ones, then invokes **outside** the lock. Keep that order — invoking user callbacks under the
   lock invites deadlock/reentrancy bugs. `Signal` is thread-safe; preserve the `mutex`.
2. **Type erasure must stay value-semantic and copyable.** `Drawable`/`Function` copy by
   `clone()`-ing the held `Concept`; moved-from wrappers are empty and **throw
   `std::runtime_error` if invoked**. Any new erased type keeps the `Concept`(virtual
   interface) + `Model<T>`(holder) + `clone()` shape and the empty-state check.
3. **CRTP is compile-time, not virtual.** Mixins access the derived type via
   `static_cast<Derived&>(*this)` / the `self()` helper — there are **no virtual functions**.
   A mixin's required hooks (e.g. `do_serialize`, `to_string`, `compare_key`) are a
   compile-time contract on `Derived`; document them. `Comparable` derives `==`/`<=>` from
   `compare_key()` — don't hand-write comparisons.
4. **Visitor variants are a closed set.** `std::visit(Overload{...}, expr)` must handle **every**
   alternative in `Expr` (recursive types use `std::unique_ptr` inside the variant). If you add
   an alternative, update *every* `Overload{...}` — an unhandled type is a compile error, which
   is the intended safety net.

## C++ best practices for this module
- Keep examples minimal and canonical — this module is read as documentation.
- Prefer these patterns deliberately: CRTP when you want zero-overhead static dispatch;
  type erasure when you need value semantics + runtime flexibility without forcing a base class;
  `std::variant`+visit for closed, known alternative sets; signals for decoupled events.
- `Overload` + deduction guide is the standard "overloaded lambda" idiom — reuse it.
- Everything is `inline`/templated and header-only; no TU, no `static` globals.

## When editing
- Update `tests/patterns_tests.cpp` (tags `[patterns]`, e.g. `[patterns][observer]`) and
  `examples/patterns_demo.cpp`.
- Adding a pattern → new header + entry in the design-patterns table in
  [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).

## Neighbors
The RAII/Rule-of-Five discipline mirrors [memory](../memory/AGENTS.md) and
[core](../core/AGENTS.md). `Repository` (another pattern) lives in [database](../database/AGENTS.md).
