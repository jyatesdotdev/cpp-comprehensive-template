/// @file hpc_demo.cpp
/// @brief Demonstrates SIMD operations and parallel STL wrappers.

#include <hpc/parallel_stl.h>
#include <hpc/simd_ops.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

namespace {

using Clock = std::chrono::high_resolution_clock;

/// @brief Benchmark helper — runs @p fn @p reps times and returns average ms.
template <typename Fn>
double bench_ms(Fn fn, int reps = 100) {
    fn(); // warm-up
    auto t0 = Clock::now();
    for (int i = 0; i < reps; ++i) fn();
    auto t1 = Clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count() / reps;
}

/// @brief Generate @p n random floats in [-1, 1].
std::vector<float> random_floats(std::size_t n, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<float> v(n);
    std::generate(v.begin(), v.end(), [&] { return dist(rng); });
    return v;
}

} // namespace

int main() {
    constexpr std::size_t N = 1'000'000;

    std::printf("=== HPC / SIMD Demo ===\n");
    std::printf("SIMD lane width: %zu floats\n", hpc::kSimdWidth);
    std::printf("Parallel STL:    %s\n\n", HPC_HAS_PARALLEL_STL ? "yes" : "no (sequential fallback)");

    // ── SIMD dot product benchmark ──────────────────────────────────
    auto a = random_floats(N, 1);
    auto b = random_floats(N, 2);

    float scalar_result = 0, simd_result = 0;
    double scalar_ms = bench_ms([&] { scalar_result = hpc::scalar_dot(a.data(), b.data(), N); });
    double simd_ms   = bench_ms([&] { simd_result   = hpc::simd_dot(a.data(), b.data(), N); });

    std::printf("Dot product (%zu floats):\n", N);
    std::printf("  scalar: %.4f ms  (result: %.6f)\n", scalar_ms, scalar_result);
    std::printf("  SIMD:   %.4f ms  (result: %.6f)\n", simd_ms, simd_result);
    std::printf("  speedup: %.2fx\n\n", scalar_ms / simd_ms);

    // ── SIMD element-wise add ───────────────────────────────────────
    std::vector<float> dst(N);
    double add_ms = bench_ms([&] { hpc::simd_add(dst.data(), a.data(), b.data(), N); });
    std::printf("SIMD add (%zu floats): %.4f ms\n\n", N, add_ms);

    // ── Parallel STL sort vs std::sort ──────────────────────────────
    auto data = random_floats(N, 3);
    auto copy = data;

    double seq_ms = bench_ms([&] {
        data = copy;
        std::sort(data.begin(), data.end());
    }, 10);

    double par_ms = bench_ms([&] {
        data = copy;
        hpc::par_sort(data.begin(), data.end());
    }, 10);

    std::printf("Sort %zu floats:\n", N);
    std::printf("  std::sort:  %.2f ms\n", seq_ms);
    std::printf("  par_sort:   %.2f ms\n", par_ms);
    std::printf("  speedup:    %.2fx\n\n", seq_ms / par_ms);

    // ── Parallel transform-reduce ───────────────────────────────────
    double tr_ms = bench_ms([&] {
        hpc::par_transform_reduce(
            a.begin(), a.end(), 0.0f, std::plus<>{},
            [](float x) { return x * x; });
    });
    std::printf("par_transform_reduce (sum of squares): %.4f ms\n", tr_ms);

    return 0;
}
