# .github/workflows/ — Continuous Integration

Scope: `.github/workflows/`. Parent: [root AGENTS.md](../../AGENTS.md). Runs on push / PR to `main`.

## Workflows & jobs (what gates a merge)
### `ci.yml` — "CI"
- **Build & Test** (`ubuntu-24.04`, `macos-14`, `windows-2022`): bootstrap vcpkg into
  `$GITHUB_WORKSPACE/.vcpkg` → `cmake --preset ci` → build → `ctest --preset ci`.
  The `ci` preset is Release, `BUILD_TESTS=ON`, `BUILD_EXAMPLES=OFF`, **`ENABLE_WERROR=ON`**.
- **Coverage (≥80%)**: `coverage` preset → run tests → `lcov` capture → **fails if line
  coverage < 80%**. `/usr/*`, `tests/`, `vcpkg_installed/`, `_deps/` are excluded from the total.

### `security.yml` — "Security Scanning"
- **Static Analysis**: `security-scan` preset (clang-tidy + cppcheck); greps the build log and
  **fails on any `cert-*` or `clang-analyzer-security` warning**.
- **ASan + UBSan**: `asan-ubsan` preset → build → test (leak detection on).
- **ThreadSanitizer**: `tsan` preset → build → test (data-race detection).
- **MemorySanitizer**: `msan` preset on Clang (`CC=clang CXX=clang++`). Requires instrumented
  deps; vcpkg libraries are not MSan-built, so this job can report false positives on
  third-party code — still a gate for *our* objects.
- **Trivy**: filesystem vulnerability scan → uploads SARIF (needs `security-events: write`).

## Rules for editing CI (learn from the git history)
1. **Presets are the interface.** Jobs invoke `cmake --preset <x>` / `ctest --preset <x>` — they
   must reference presets that exist in `CMakePresets.json`. Keep the three in lockstep
   (`CMakePresets.json` ↔ workflows ↔ [cmake/Security.cmake](../../cmake/AGENTS.md)); a prior
   commit specifically realigned them.
2. **Don't weaken the gates** without explicit intent: the 80% coverage threshold, the
   security-finding grep, and the sanitizer jobs are the point of this template. If you must
   tolerate a tool quirk, scope it narrowly (recent fixes used `lcov --ignore-errors
   mismatch,empty` and `|| true` only on the non-gating `--list` step — not on the threshold check).
3. **Least privilege.** `security.yml` declares `permissions: security-events: write, contents:
   read`. Add scopes only when a step needs them (SARIF upload needed `security-events`).
4. **Pin/trust actions deliberately.** `actions/checkout@v4`, `codeql-action/upload-sarif@v3`,
   and `trivy-action@0.28.0` are version-pinned. Do not float `@master`.
5. Sanitizers are **mutually exclusive** (ASan/TSan/MSan) — that's why ASan+UBSan and TSan are
   separate jobs. Don't merge them.

## Reproduce a CI job locally
```bash
cmake --preset ci          && cmake --build --preset ci          && ctest --preset ci
cmake --preset coverage    && cmake --build --preset coverage    && ctest --preset coverage
cmake --preset security-scan && cmake --build --preset security-scan
cmake --preset asan-ubsan  && cmake --build --preset asan-ubsan  && ctest --preset asan-ubsan
cmake --preset tsan        && cmake --build --preset tsan        && ctest --preset tsan
CC=clang CXX=clang++ cmake --preset msan && cmake --build --preset msan && ctest --preset msan
```
Everything needs `VCPKG_ROOT` set. Background: [docs/SECURITY_SCANNING.md](../../docs/SECURITY_SCANNING.md).
