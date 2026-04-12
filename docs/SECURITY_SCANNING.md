# Security Scanning

This project integrates static analysis, runtime sanitizers, and memory checking into the CMake build system. All tools are opt-in and controlled via CMake options or presets.

## Quick Start

```bash
# Static analysis (clang-tidy + cppcheck)
cmake --preset security-scan
cmake --build --preset security-scan

# AddressSanitizer + UBSan (most common for development)
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan

# Individual sanitizers
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
cmake --preset ubsan && cmake --build --preset ubsan && ctest --preset ubsan
cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan
cmake --preset msan && cmake --build --preset msan && ctest --preset msan
```

## Tools Overview

| Tool | CMake Option | Preset | What It Detects |
|------|-------------|--------|-----------------|
| clang-tidy | `ENABLE_CLANG_TIDY` | `security-scan` | CERT violations, security bugs, code quality |
| cppcheck | `ENABLE_CPPCHECK` | `security-scan` | UB, buffer overflows, memory leaks |
| AddressSanitizer | `ENABLE_ASAN` | `asan` | Out-of-bounds, use-after-free, double-free, leaks |
| UBSan | `ENABLE_UBSAN` | `ubsan` | Signed overflow, null deref, type violations |
| ThreadSanitizer | `ENABLE_TSAN` | `tsan` | Data races |
| MemorySanitizer | `ENABLE_MSAN` | `msan` | Uninitialized memory reads (Clang only) |
| include-what-you-use | `ENABLE_IWYU` | — | Unnecessary/missing headers |
| Valgrind | `ENABLE_VALGRIND` | — | Memory errors, leaks (Linux only) |

## Static Analysis

### clang-tidy

Configured via `.clang-tidy` in the project root. Runs automatically during compilation when enabled.

**Enabled check groups:**
- `cert-*` — CERT C++ secure coding rules
- `bugprone-*` — common bug patterns
- `clang-analyzer-security.*` — security-specific analysis
- `clang-analyzer-core.*` — null deref, divide-by-zero, uninitialized
- `clang-analyzer-cplusplus.*` — new/delete, move semantics
- `concurrency-*` — thread-safety issues
- `cppcoreguidelines-*`, `modernize-*`, `performance-*`, `readability-*`

**Build-breaking checks** (WarningsAsErrors):
- All `cert-*` and `clang-analyzer-security.*` checks
- `concurrency-*`
- Critical bugprone checks: `use-after-move`, `dangling-handle`, `infinite-loop`, `sizeof-expression`
- `clang-analyzer-core.NullDereference`, `clang-analyzer-cplusplus.NewDeleteLeaks`

**Manual invocation:**
```bash
cmake -DENABLE_CLANG_TIDY=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### cppcheck

Runs automatically during compilation when enabled. Configured with:
- `--enable=warning,performance,portability`
- `--error-exitcode=1` (fails the build on findings)
- `--std=c++20`
- `--inline-suppr` (supports `// cppcheck-suppress` comments)

## Runtime Sanitizers

Each sanitizer gets its own CMake preset with a dedicated build directory (`build/<preset-name>`).

### Sanitizer Compatibility

Sanitizers have mutual exclusion constraints enforced at configure time:

| | ASan | UBSan | TSan | MSan |
|------|------|-------|------|------|
| ASan | — | ✅ | ❌ | ❌ |
| UBSan | ✅ | — | ✅ | ✅ |
| TSan | ❌ | ✅ | — | ❌ |
| MSan | ❌ | ✅ | ❌ | — |

MSan requires Clang (not supported by GCC).

### Runtime Options

Test presets automatically set sanitizer runtime environment variables:

| Preset | Environment |
|--------|-------------|
| `asan` | `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` |
| `ubsan` | `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1` |
| `tsan` | `TSAN_OPTIONS=halt_on_error=1` |
| `msan` | `MSAN_OPTIONS=halt_on_error=1` |
| `asan-ubsan` | Both ASan and UBSan options set |

### Valgrind

Valgrind uses CTest's built-in memcheck support. Linux only.

```bash
cmake -DENABLE_VALGRIND=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
ctest -T memcheck
```

Options: `--leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1`

### include-what-you-use

Analyzes header dependencies during compilation:

```bash
cmake -DENABLE_IWYU=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

## Suppression Patterns

### clang-tidy

```cpp
// Suppress on current line
int x = legacy_func(); // NOLINT(cert-err58-cpp)

// Suppress on next line
// NOLINTNEXTLINE(bugprone-narrowing-conversions)
int y = some_double;
```

File-level suppression: create `.clang-tidy-ignore` with glob patterns (one per line).

Global suppression: add `-check-name` to the `Checks` list in `.clang-tidy`.

### cppcheck

```cpp
// cppcheck-suppress uninitvar
int x;
```

### Sanitizers

Sanitizers support suppression files via environment variables:

```bash
# ASan
export ASAN_OPTIONS="suppressions=asan_suppressions.txt"

# LSan (leak sanitizer, part of ASan)
export LSAN_OPTIONS="suppressions=lsan_suppressions.txt"

# TSan
export TSAN_OPTIONS="suppressions=tsan_suppressions.txt"
```

Suppression file format:
```
# asan_suppressions.txt
interceptor_via_fun:third_party_function

# lsan_suppressions.txt
leak:ThirdPartyLib::init

# tsan_suppressions.txt
race:ThirdPartyLib::sharedState
```

## CI Integration

See `.github/workflows/security.yml` for a complete GitHub Actions example that:
- Runs clang-tidy + cppcheck on every push/PR
- Runs ASan + UBSan tests
- Fails the pipeline on high-severity findings

## Architecture

```
cmake/Security.cmake    ← Centralized module (clang-tidy, cppcheck, iwyu, valgrind)
CMakeLists.txt          ← Sanitizer options + flag composition
CMakePresets.json       ← Presets for each sanitizer and security-scan
.clang-tidy             ← Security-focused clang-tidy rules
```

All security tools are OFF by default. Enable them via presets or `-D` flags to avoid slowing normal development builds.

## See Also

- [Toolchain](TOOLCHAIN.md) — Installing clang-tidy, cppcheck, and sanitizer-capable compilers
- [Architecture](ARCHITECTURE.md) — `cmake/Security.cmake` module and build options
- [Cross-Platform Build](cross_platform_build.md) — Sanitizer preset usage per platform
- [Tutorial](TUTORIAL.md) — Running security scans step-by-step
