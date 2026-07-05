# docs/ — project documentation

Scope: `docs/`. Parent: [root AGENTS.md](../AGENTS.md). Format: Markdown (also fed to Doxygen
when `BUILD_DOCS=ON`).

## Business role
The narrative/reference documentation that complements the in-code Doxygen. These files are
**part of the contract with users** — when you change code, the matching doc must stay true.
The root `README.md` and `docs/ARCHITECTURE.md` link out to everything here.

## Index of guides
| File | Topic | Keep in sync with |
|---|---|---|
| `ARCHITECTURE.md` | Layout, module/namespace/target table, dependency graph, patterns | root `CMakeLists.txt`, every module |
| `TOOLCHAIN.md` | Required tools, compiler versions, IDE setup | `vcpkg.json`, `CMakePresets.json` |
| `TUTORIAL.md` | New-developer walkthrough (clone → build → test → extend) | build presets, examples |
| `EXTENDING.md` | Adding libraries, examples, tests, deps, presets | `CMakeLists.txt`, `vcpkg.json` |
| `best_practices.md` | Modern C++17/20/23 coding guidelines | the whole codebase's idioms |
| `api_design.md` | REST + C++ API design | [api](../include/api/AGENTS.md) |
| `cli.md` | CLI design with CLI11 | [cli](../include/cli/AGENTS.md) |
| `hpc_optimization.md` | SIMD + parallel algorithms | [hpc](../include/hpc/AGENTS.md) |
| `rendering_pipeline.md` | OpenGL/Vulkan graphics architecture | [rendering](../include/rendering/AGENTS.md) |
| `cross_platform_build.md` | Per-platform build notes | `CMakePresets.json`, `Dockerfile` |
| `SECURITY_SCANNING.md` | Static analysis, sanitizers, CI security | `cmake/Security.cmake`, `.clang-tidy`, CI |
| `third_party_integration.md` | vcpkg / fmt / spdlog / JSON usage | `vcpkg.json`, `CMakeLists.txt` |

## Rules for editing docs
1. **Docs must match reality.** If a change makes a doc statement false (a preset name, a target
   type, a module's dependency, a coverage threshold), fix the doc in the same change. Stale
   docs are treated as bugs.
2. **Cross-link, don't duplicate.** Each doc ends with a "See Also"/related-docs section; keep
   those links valid. Facts that live in code or `AGENTS.md` should be linked, not copy-pasted.
3. **Match the house voice:** concise, practical, example-driven; fenced code blocks tagged with
   a language; tables for option/preset/module matrices (see existing files).
4. **These `AGENTS.md` files are for agents, not end users** — put user-facing guidance in the
   normal docs, and keep the accurate "business rules / invariants" in the relevant module's
   `AGENTS.md`. Don't conflate the two audiences.
5. Prefer relative links (`../include/api/AGENTS.md`, `best_practices.md`) so they resolve on
   GitHub and in local editors.

## Note
`docs/` is **not** a build input for code and has no tests; correctness is by review. When a
module's behavior or public contract changes, check the "Keep in sync with" column above.
