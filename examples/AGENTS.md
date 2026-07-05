# examples/ — runnable demo programs

Scope: `examples/`. Parent: [root AGENTS.md](../AGENTS.md).

## Purpose & business rules
Each module ships **one runnable demo**, `<module>_demo.cpp`, that shows its API in a
realistic mini-program. Demos double as living documentation and smoke tests — they **must
compile and run**, and must stay truthful to the current API (update them when you change a
module). `hello_world.cpp` (the first-run program) and `third_party_demo.cpp` link `core`.

## Adding / editing a demo
- Register in `CMakeLists.txt`:
  ```cmake
  add_executable(<name>_demo <name>_demo.cpp)
  target_link_libraries(<name>_demo PRIVATE <module>)
  ```
- **Gate demos whose module is conditional**, mirroring the target's own condition:
  - `api_demo` → `if(httplib_FOUND)`
  - `database_demo` → `if(SQLite3_FOUND)`
  - `rendering_demo` → `if(ENABLE_RENDERING AND OpenGL_FOUND AND glfw3_FOUND)`
  Never link a target that may not exist without its guard, or the whole configure step breaks.
- Examples build only when `BUILD_EXAMPLES=ON` (default; **OFF** in the `ci`/`coverage` presets —
  so a broken example won't be caught by those jobs, only by a local/default build).

## Conventions
- Start with `/// @file` + `/// @brief`. Keep demos **self-contained, small, and readable** —
  they are teaching material; favor clarity over cleverness.
- Include module headers by their public path (`#include "cli/cli_helpers.h"`).
- A demo's `main` should return non-zero on failure so it can act as a smoke test.
- Same house style as the rest of the repo: C++20, 4-space/100-col, warning-clean, snake_case
  free functions, `PascalCase` types. See [root AGENTS.md](../AGENTS.md).
- Feature-specific code (e.g. Boost) stays behind its macro (`#ifdef HAS_BOOST`) as in
  `cli_demo.cpp`; the default path must always build.

## Build & run
```bash
cmake --preset default && cmake --build build/default
./build/default/examples/hello_world
./build/default/examples/cli_demo format all
```

## Per-module context
Each demo maps to a module — read that module's `AGENTS.md` (e.g.
[include/cli/AGENTS.md](../include/cli/AGENTS.md)) for the rules the demo must respect.
