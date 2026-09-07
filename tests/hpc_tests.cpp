/// @file hpc_tests.cpp
/// @brief Unit tests for SIMD helpers and parallel-STL wrappers.

#include "hpc/parallel_stl.h"
#include "hpc/simd_ops.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numeric>
#include <vector>

using Catch::Matchers::WithinAbs;

TEST_CASE("simd_add matches scalar addition", "[hpc][simd]") {
    std::vector<float> a{1.f, 2.f, 3.f, 4.f, 5.f};
    std::vector<float> b{10.f, 20.f, 30.f, 40.f, 50.f};
    std::vector<float> dst(a.size(), 0.f);
    hpc::simd_add(dst.data(), a.data(), b.data(), a.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        REQUIRE_THAT(dst[i], WithinAbs(a[i] + b[i], 1e-6f));
    }
}

TEST_CASE("simd_mul matches scalar multiplication", "[hpc][simd]") {
    std::vector<float> a{1.f, 2.f, 3.f, 4.f};
    std::vector<float> b{2.f, 2.f, 2.f, 2.f};
    std::vector<float> dst(a.size(), 0.f);
    hpc::simd_mul(dst.data(), a.data(), b.data(), a.size());
    REQUIRE_THAT(dst[3], WithinAbs(8.f, 1e-6f));
}

TEST_CASE("simd_dot matches scalar_dot", "[hpc][simd]") {
    std::vector<float> a(17, 1.5f);
    std::vector<float> b(17, 2.0f);
    auto simd = hpc::simd_dot(a.data(), b.data(), a.size());
    auto scalar = hpc::scalar_dot(a.data(), b.data(), a.size());
    REQUIRE_THAT(simd, WithinAbs(scalar, 1e-4f));
}

TEST_CASE("par_sort orders a vector", "[hpc][pstl]") {
    std::vector<int> v{5, 1, 4, 2, 3};
    hpc::par_sort(v.begin(), v.end());
    REQUIRE(v == std::vector<int>{1, 2, 3, 4, 5});
}

TEST_CASE("par_reduce sums", "[hpc][pstl]") {
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 1);
    REQUIRE(hpc::par_reduce(v.begin(), v.end(), 0) == 55);
}
