# High-Performance Computing & Optimization Guide

## SIMD (Single Instruction, Multiple Data)

### What It Is
SIMD processes multiple data elements in a single CPU instruction. A 256-bit AVX register holds 8 floats, so one `_mm256_add_ps` replaces 8 scalar additions.

### Platform Support

| ISA | Register Width | Floats/Op | Availability |
|-----|---------------|-----------|--------------|
| SSE2 | 128-bit | 4 | All x86-64 |
| AVX2 | 256-bit | 8 | Intel Haswell+ / AMD Excavator+ |
| AVX-512 | 512-bit | 16 | Intel Skylake-X+ / AMD Zen 4+ |
| NEON | 128-bit | 4 | All AArch64 (Apple Silicon, ARM servers) |

### Writing Portable SIMD Code

The template in `include/hpc/simd_ops.h` demonstrates the compile-time detection pattern:

```cpp
#if defined(__AVX2__)
    // 8-wide float operations
#elif defined(__SSE2__)
    // 4-wide float operations
#elif defined(__ARM_NEON)
    // 4-wide float operations (ARM)
#endif
    // scalar tail loop for remaining elements
```

Key principles:
- Always include a scalar fallback for portability
- Handle tail elements (when N is not a multiple of lane width)
- Use `__restrict` to promise no aliasing (enables better codegen)
- Prefer unaligned loads (`_mm256_loadu_ps`) for flexibility; aligned loads are rarely faster on modern CPUs

### Compiler Flags

```bash
# x86 — enable AVX2 + FMA
cmake -DCMAKE_CXX_FLAGS="-mavx2 -mfma" ..

# ARM (Apple Silicon) — NEON is always available
# No extra flags needed

# Let the compiler target the build machine
cmake -DCMAKE_CXX_FLAGS="-march=native" ..
```

### When to Use SIMD

Use SIMD for:
- Tight loops over contiguous float/int arrays (dot products, image processing, audio)
- Operations that map cleanly to vector instructions (add, mul, FMA, min/max)

Avoid manual SIMD when:
- The compiler auto-vectorises well (check with `-fopt-info-vec` or `-Rpass=loop-vectorize`)
- Data access is irregular or pointer-chasing (linked lists, trees)
- The loop body has complex branching

## C++17 Parallel Execution Policies

### Overview

C++17 added execution policies to standard algorithms:

```cpp
#include <execution>
std::sort(std::execution::par_unseq, v.begin(), v.end());
```

| Policy | Meaning |
|--------|---------|
| `seq` | Sequential (default) |
| `par` | Parallel across threads |
| `par_unseq` | Parallel + vectorised (SIMD) |
| `unseq` | Vectorised only (C++20) |

### Availability

- MSVC: Built-in support
- GCC/libstdc++: Requires Intel TBB (`libtbb-dev`)
- Clang/libc++: Limited support; use TBB backend or manual threading

The wrappers in `include/hpc/parallel_stl.h` compile-time detect availability and fall back to sequential.

### When Parallel STL Helps

Parallel STL shines when:
- N > ~10,000 elements (thread overhead dominates for small N)
- Per-element work is non-trivial (transform, reduce, sort)
- Data fits in cache or streams well from memory

## General Optimization Checklist

### 1. Measure First
```bash
# Profile with perf (Linux)
perf stat ./hpc_demo
perf record -g ./hpc_demo && perf report

# Profile with Instruments (macOS)
xcrun xctrace record --template "Time Profiler" --launch ./hpc_demo
```

### 2. Data Layout
- Prefer Structure of Arrays (SoA) over Array of Structures (AoS) for SIMD
- Align hot data to cache lines (64 bytes): `alignas(64) float data[N];`
- Minimise pointer indirection; use contiguous containers (`std::vector`)

### 3. Memory Hierarchy
- L1 cache: ~32 KB, ~1 ns latency
- L2 cache: ~256 KB, ~4 ns
- L3 cache: ~8-32 MB, ~12 ns
- DRAM: ~50-100 ns

Keep working sets small. Tile/block loops to fit in L1/L2.

### 4. Compiler Optimisation
```bash
# Release build with LTO
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..

# Check what the compiler vectorised
g++ -O3 -march=native -fopt-info-vec-optimized file.cpp
clang++ -O3 -march=native -Rpass=loop-vectorize file.cpp
```

### 5. Avoid Common Pitfalls
- **False sharing**: Pad shared counters to cache-line boundaries
- **Branch misprediction**: Use branchless techniques or `[[likely]]`/`[[unlikely]]`
- **Aliasing**: Use `__restrict` or `-fstrict-aliasing`
- **Denormals**: Set flush-to-zero mode for float-heavy code (`_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON)`)

## Benchmarking Tips

- Warm up caches before timing
- Run multiple iterations and report median
- Use `volatile` or `benchmark::DoNotOptimize` to prevent dead-code elimination
- Compare against a known baseline (scalar, single-threaded)
- Test with realistic data sizes that match your production workload

## See Also

- [Architecture](ARCHITECTURE.md) — HPC module in the dependency graph
- [Best Practices](best_practices.md) — Performance guidelines and modern C++ patterns
- [Toolchain](TOOLCHAIN.md) — Compiler flags and parallel STL availability
- [Cross-Platform Build](cross_platform_build.md) — Platform-specific compiler notes
