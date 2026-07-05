# src/ — Implementation files

Scope: `src/`. Parent: [root AGENTS.md](../AGENTS.md).

`src/` mirrors `include/`, but **only two modules actually have `.cpp` files**, because only
`core` and `memory` are compiled STATIC libraries. Every other module is header-only
(`INTERFACE`), so its logic lives entirely in `include/<module>/` and it has **no `src/`
implementation** (the empty `src/<module>/` dirs are placeholders/mirrors).

| Path | Backs target | Notes |
|---|---|---|
| `src/core/app.cpp` | `core` (STATIC) | Defines `App::Impl` (PImpl) — see [src/core/AGENTS.md](core/AGENTS.md) |
| `src/memory/resource_handle.cpp` | `memory` (STATIC) | Defines `CloseFd` with POSIX detection — see [src/memory/AGENTS.md](memory/AGENTS.md) |
| `src/{api,concurrency,database,etl,hpc,patterns,rendering,simulation}/` | — | **Empty.** Those modules are header-only; put code in `include/<module>/`. |

## When you need a `.cpp` here

Add a translation unit only when a module is (or becomes) a compiled library:
1. The module must be a STATIC/OBJECT target in the root `CMakeLists.txt` (not `INTERFACE`).
2. Add the file to that target's `add_library(... src/<module>/<file>.cpp)` list.
3. `#include "<module>/<header>.h"` first, then implement; keep the header's declarations and
   Doxygen contract authoritative.
4. Converting a header-only module to a compiled one is a build-system change — see
   [docs/EXTENDING.md](../docs/EXTENDING.md) and update the module's `AGENTS.md`.

## Conventions
Same house rules as everywhere: C++20, `#pragma once` is header-only (source files need no
guard), 4-space indent / 100 cols, `PascalCase` types, `lower_snake_case` functions, private
members end with `_`, warning-clean under strict flags. Guard platform/optional code behind
the correct macro (`HAS_SPDLOG`, `HAS_POSIX_CLOSE`, …) and keep the fallback path compiling.
See the [root AGENTS.md](../AGENTS.md) for the full list.
