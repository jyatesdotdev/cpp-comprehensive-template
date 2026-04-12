#pragma once

/// @file simd_ops.h
/// @brief Portable SIMD wrappers for common float-vector operations.
///        Auto-detects SSE/AVX (x86) or NEON (ARM) at compile time.

#include <array>
#include <cstddef>
#include <numeric>

// ── Platform detection ──────────────────────────────────────────────
#if defined(__AVX2__)
#define HPC_SIMD_AVX2 1
#include <immintrin.h>
#elif defined(__SSE2__)
#define HPC_SIMD_SSE2 1
#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define HPC_SIMD_NEON 1
#include <arm_neon.h>
#endif

namespace hpc {

/// @brief SIMD lane width — number of floats processed per instruction.
///
/// Compile-time constant: 8 (AVX2), 4 (SSE2/NEON), or 1 (scalar fallback).
#if defined(HPC_SIMD_AVX2)
inline constexpr std::size_t kSimdWidth = 8;
#elif defined(HPC_SIMD_SSE2) || defined(HPC_SIMD_NEON)
inline constexpr std::size_t kSimdWidth = 4;
#else
inline constexpr std::size_t kSimdWidth = 1; // scalar fallback
#endif

/// @brief Element-wise addition of two float arrays using SIMD intrinsics.
///
/// Computes `dst[i] = a[i] + b[i]` for all `i` in `[0, n)`.
/// Tail elements beyond the SIMD lane width are handled with scalar code.
///
/// @param[out] dst  Destination array (must hold at least @p n floats).
/// @param[in]  a    First source array.
/// @param[in]  b    Second source array.
/// @param[in]  n    Number of elements.
inline void simd_add(float* __restrict dst, const float* __restrict a,
                     const float* __restrict b, std::size_t n) {
    std::size_t i = 0;
#if defined(HPC_SIMD_AVX2)
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        _mm256_storeu_ps(dst + i, _mm256_add_ps(va, vb));
    }
#elif defined(HPC_SIMD_SSE2)
    for (; i + 4 <= n; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        _mm_storeu_ps(dst + i, _mm_add_ps(va, vb));
    }
#elif defined(HPC_SIMD_NEON)
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        vst1q_f32(dst + i, vaddq_f32(va, vb));
    }
#endif
    for (; i < n; ++i) dst[i] = a[i] + b[i]; // scalar tail
}

/// @brief Element-wise multiplication of two float arrays using SIMD intrinsics.
///
/// Computes `dst[i] = a[i] * b[i]` for all `i` in `[0, n)`.
///
/// @param[out] dst  Destination array (must hold at least @p n floats).
/// @param[in]  a    First source array.
/// @param[in]  b    Second source array.
/// @param[in]  n    Number of elements.
inline void simd_mul(float* __restrict dst, const float* __restrict a,
                     const float* __restrict b, std::size_t n) {
    std::size_t i = 0;
#if defined(HPC_SIMD_AVX2)
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        _mm256_storeu_ps(dst + i, _mm256_mul_ps(va, vb));
    }
#elif defined(HPC_SIMD_SSE2)
    for (; i + 4 <= n; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        _mm_storeu_ps(dst + i, _mm_mul_ps(va, vb));
    }
#elif defined(HPC_SIMD_NEON)
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        vst1q_f32(dst + i, vmulq_f32(va, vb));
    }
#endif
    for (; i < n; ++i) dst[i] = a[i] * b[i];
}

/// @brief SIMD-accelerated dot product of two float arrays.
///
/// Computes `sum(a[i] * b[i])` for `i` in `[0, n)`.
///
/// @param[in] a  First source array.
/// @param[in] b  Second source array.
/// @param[in] n  Number of elements.
/// @return The dot product as a single float.
inline float simd_dot(const float* a, const float* b, std::size_t n) {
    float sum = 0.0f;
    std::size_t i = 0;
#if defined(HPC_SIMD_AVX2)
    __m256 acc = _mm256_setzero_ps();
    for (; i + 8 <= n; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        acc = _mm256_fmadd_ps(va, vb, acc); // requires FMA (implied by AVX2 in practice)
    }
    // horizontal sum of 8 floats
    __m128 lo = _mm256_castps256_ps128(acc);
    __m128 hi = _mm256_extractf128_ps(acc, 1);
    __m128 s  = _mm_add_ps(lo, hi);
    s = _mm_hadd_ps(s, s);
    s = _mm_hadd_ps(s, s);
    sum = _mm_cvtss_f32(s);
#elif defined(HPC_SIMD_SSE2)
    __m128 acc = _mm_setzero_ps();
    for (; i + 4 <= n; i += 4) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        acc = _mm_add_ps(acc, _mm_mul_ps(va, vb));
    }
    // horizontal sum
    alignas(16) float tmp[4];
    _mm_store_ps(tmp, acc);
    sum = tmp[0] + tmp[1] + tmp[2] + tmp[3];
#elif defined(HPC_SIMD_NEON)
    float32x4_t acc = vdupq_n_f32(0.0f);
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        acc = vmlaq_f32(acc, va, vb);
    }
    sum = vaddvq_f32(acc);
#endif
    for (; i < n; ++i) sum += a[i] * b[i]; // scalar tail
    return sum;
}

/// @brief Scalar dot product (no SIMD) for benchmarking comparison.
///
/// @param[in] a  First source array.
/// @param[in] b  Second source array.
/// @param[in] n  Number of elements.
/// @return The dot product as a single float.
inline float scalar_dot(const float* a, const float* b, std::size_t n) {
    float sum = 0.0f;
    for (std::size_t i = 0; i < n; ++i) sum += a[i] * b[i];
    return sum;
}

} // namespace hpc
