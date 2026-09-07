# simulation — ECS, physics (Verlet), numerical integration

Scope: `include/simulation/`. Namespace `simulation`. CMake target **`simulation` (INTERFACE /
header-only)**, no external deps. Parent: [include/AGENTS.md](../AGENTS.md).

## Business role
Data-oriented simulation building blocks: a minimal Entity-Component-System, a 2D
Verlet-integration physics world, and numerical routines. Demoed in
`examples/simulation_demo.cpp`.

## Files
- `ecs.h` — `Entity` (a `uint32_t` id) and `World`: `create`, `add<T>`, `get<T>`, `remove<T>`,
  `destroy(e)`, and `each<Ts...>(fn)`. Components live in **type-erased pools** (`std::any`
  holding an `unordered_map<Entity, T>`, keyed by `std::type_index` from `typeid`).
  **`World` requires RTTI** (`-fno-rtti` is not supported).
- `numerical.h` — `rk4` (4th-order Runge–Kutta ODE integrator) and `simpson` (Simpson's-rule
  definite integral).
- `physics.h` — `Vec2`, `Particle`, `DistanceConstraint`, and `PhysicsWorld` (Verlet step +
  constraint relaxation).

## Invariants & business rules (MUST hold)
1. **ECS storage model.** Components are plain structs. `World::add<T>` **replaces** any
   existing `T` on that entity. `get<T>` returns `T*` or `nullptr` (never throws / never
   inserts on miss). `each<Ts...>` iterates the pool of the **first** component type and skips
   entities missing any of the others (`try_get_all`). Preserve the "first pool drives
   iteration" logic and the pointer/`optional<tuple>` matching. Component pools are keyed by
   `std::type_index(typeid(T))`, so **`World` requires RTTI** — do not build with `-fno-rtti`.
2. **Entity ids are monotonic and never reused** (`next_id_++`). `destroy(e)` strips every
   component from `e` but does **not** recycle the id. Don't assume dense or reusable ids;
   `remove<T>` drops one component, `destroy` drops all of them.
3. **Physics is Verlet, so velocity is implicit** in `pos - prev`. `step(dt)` must: apply
   `accel = gravity`, integrate `pos += (pos - prev) + accel*dt*dt`, set `prev` correctly, then
   run `constraint_iters` (default 4) relaxation passes. **Pinned particles never move** —
   honor `pinned` in both integration and constraint resolution. Skip near-zero distances
   (`dist < 1e-12`) to avoid divide-by-zero. `add_constraint` captures the current distance as
   `rest_length`.
4. **`simpson`'s `n` must be even** — it force-increments odd `n`. Keep that guard.
   `rk4` returns the full `(t, y)` trajectory including the initial point; the loop bound uses
   `t < t_end - dt*0.5` to land cleanly on the end. Both take the RHS/integrand as a callable
   template param.

## C++ best practices for this module
- Data-oriented: keep components trivial/aggregate structs (cheap to copy, cache-friendly).
- Template numerical routines on the function type; take `F&&` and invoke directly (no
  `std::function` overhead in hot loops).
- Prefer `std::size_t` indices; mind `-Wconversion` when mixing with `double` math.
- `each<>` does a linear scan — fine for the template's scale; note it if you add large worlds.

## When editing
- Update `tests/simulation_tests.cpp` (tag `[simulation]`) and `examples/simulation_demo.cpp`.
- New numerical method → add alongside `rk4`/`simpson` with full Doxygen (`@pre` for
  preconditions like evenness) and a convergence/accuracy test.

## Neighbors
For heavier math throughput see [hpc](../hpc/AGENTS.md) (SIMD) and
[docs/hpc_optimization.md](../../docs/hpc_optimization.md). ECS is catalogued in
[docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).
