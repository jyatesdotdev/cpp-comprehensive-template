# include/ — Public headers (the API surface)

Scope: `include/`. Parent: [root AGENTS.md](../AGENTS.md).

Everything under `include/` is the project's **public API**. Each subdirectory is one module
mapping to `namespace <module>` and a CMake target of the same name. Headers here are added
to targets via `target_include_directories(... include)`, so consumers write
`#include "<module>/<file>.h"` (e.g. `#include "core/app.h"`).

## Subdirectories

| Dir | Namespace | Read before editing |
|---|---|---|
| `core/` | `core` | [core/AGENTS.md](core/AGENTS.md) |
| `memory/` | `memory` | [memory/AGENTS.md](memory/AGENTS.md) |
| `concurrency/` | `concurrency` | [concurrency/AGENTS.md](concurrency/AGENTS.md) |
| `hpc/` | `hpc` | [hpc/AGENTS.md](hpc/AGENTS.md) |
| `etl/` | `etl` | [etl/AGENTS.md](etl/AGENTS.md) |
| `api/` | `api` | [api/AGENTS.md](api/AGENTS.md) |
| `database/` | `database` | [database/AGENTS.md](database/AGENTS.md) |
| `patterns/` | `patterns` | [patterns/AGENTS.md](patterns/AGENTS.md) |
| `rendering/` | `rendering`, `rendering::gl` | [rendering/AGENTS.md](rendering/AGENTS.md) |
| `simulation/` | `simulation` | [simulation/AGENTS.md](simulation/AGENTS.md) |
| `cli/` | `cli`, `cli::fmt` | [cli/AGENTS.md](cli/AGENTS.md) |

## Rules for any header in this tree

- Start with `#pragma once`, then a `/// @file` + `/// @brief` banner (match neighbors).
- **Most modules are header-only (`INTERFACE` targets).** For those (everything except
  `core` and `memory`), the *full implementation lives in the header* — usually `inline`
  free functions or templates. There is no `.cpp` to hide logic in.
- `core` and `memory` are STATIC libs: declare in the header, implement in
  [`src/<module>/`](../src/AGENTS.md). Keep the header declaration and the `.cpp` in sync.
- Keep includes minimal and standard-library-first; `IncludeBlocks: Regroup` will reorder
  them. Include what you use — don't rely on transitive includes.
- Document `@throws` for anything that can throw; mark non-throwing accessors `noexcept`.
- Templates and `inline` functions must be **fully defined here** (they have no TU).
- Guard optional-dependency code with the right macro (`HAS_JSON`, `ENABLE_GL`, …) and make
  sure the header still compiles with the macro undefined.

## Naming & style
Types `PascalCase`; functions/methods `lower_snake_case`; private members trailing `_`;
namespaces `lower_case`. 4-space indent, 100 columns. Full conventions in the
[root AGENTS.md](../AGENTS.md).
