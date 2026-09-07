# cmake/ — custom CMake modules

Scope: `cmake/`. Parent: [root AGENTS.md](../AGENTS.md).

## Contents
- `Security.cmake` — centralizes the static-analysis / dynamic-analysis tooling toggles.
  `include(cmake/Security.cmake)` runs near the top of the root `CMakeLists.txt` (after
  `project()`).
- `CppComprehensiveTemplateConfig.cmake.in` — `configure_package_config_file` input for
  `find_package(CppComprehensiveTemplate)`. FetchContent/`project_warnings` stay out of the
  export set (`BUILD_INTERFACE`).

## What `Security.cmake` provides (business role)
Opt-in options, each wired to a tool that is *found at configure time* (a warning, not an error,
if the tool is missing):
| Option | Effect |
|---|---|
| `ENABLE_CLANG_TIDY` | Sets `CMAKE_CXX_CLANG_TIDY` (uses the repo `.clang-tidy`; `cert-*` + `clang-analyzer-security.*` as errors) |
| `ENABLE_CPPCHECK` | Sets `CMAKE_CXX_CPPCHECK` (`warning,performance,portability`, `--error-exitcode=1`, C++20) |
| `ENABLE_IWYU` | Sets `CMAKE_CXX_INCLUDE_WHAT_YOU_USE` |
| `ENABLE_VALGRIND` | Configures a CTest `memcheck` target (`--leak-check=full … --error-exitcode=1`) |

These back the **`security-scan`** preset (clang-tidy + cppcheck) and are exercised by the
security CI workflow. See [docs/SECURITY_SCANNING.md](../docs/SECURITY_SCANNING.md).

## Rules for editing CMake here (and in root/subdir CMakeLists)
1. **Keep tools optional and non-fatal at configure time.** Use `find_program(...)` and warn if
   absent (`message(WARNING ...)`) — never hard-fail configuration because a dev lacks cppcheck.
   The *findings* are what fail CI, not the tool's absence.
2. **Don't lower the security bar.** `cert-*` and `clang-analyzer-security.*` are warnings-as-
   errors by contract; keep them. Coordinate any change with `.clang-tidy` and the CI grep in
   `.github/workflows/security.yml`.
3. Prefer target-scoped settings; guard compiler-specific flags with generator expressions
   (`$<$<CXX_COMPILER_ID:GNU,Clang>:…>`) as the root `CMakeLists.txt` does for warnings/sanitizers.
4. **Keep presets in sync.** New options usually need a matching entry in `CMakePresets.json`
   *and* the CI workflow that runs it — the three drift easily. A past commit fixed exactly this
   ("align workflow presets with CMakePresets.json").
5. Modules link `project_warnings` + `project_sanitizers` (defined in root `CMakeLists.txt`) —
   don't bypass them for new targets.

## Verify a change
```bash
cmake --preset security-scan && cmake --build build/security-scan   # clang-tidy + cppcheck
cmake --preset default -DENABLE_VALGRIND=ON && ctest -T memcheck --test-dir build/default
```

## Neighbors
Root build graph & options: [root AGENTS.md](../AGENTS.md) and `CMakeLists.txt`.
Adding deps/targets/presets: [docs/EXTENDING.md](../docs/EXTENDING.md).
