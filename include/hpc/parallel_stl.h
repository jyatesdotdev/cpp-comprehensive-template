#pragma once

/// @file parallel_stl.h
/// @brief Convenience wrappers around C++17 parallel execution policies.
///        Falls back to sequential execution when parallel STL is unavailable.

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

#if __has_include(<execution>)
#include <execution>
#define HPC_HAS_PARALLEL_STL 1
#else
#define HPC_HAS_PARALLEL_STL 0
#endif

namespace hpc {

/// @brief Parallel sort using `std::execution::par_unseq` when available.
///
/// @tparam It   Random-access iterator type.
/// @tparam Cmp  Comparison functor (defaults to `std::less<>`).
/// @param begin Iterator to the first element.
/// @param end   Iterator past the last element.
/// @param cmp   Comparison object.
template <typename It, typename Cmp = std::less<>>
void par_sort(It begin, It end, Cmp cmp = {}) {
#if HPC_HAS_PARALLEL_STL
    std::sort(std::execution::par_unseq, begin, end, cmp);
#else
    std::sort(begin, end, cmp);
#endif
}

/// @brief Parallel transform using `std::execution::par_unseq` when available.
///
/// @tparam InIt  Input iterator type.
/// @tparam OutIt Output iterator type.
/// @tparam Fn    Unary function object type.
/// @param begin  Iterator to the first input element.
/// @param end    Iterator past the last input element.
/// @param out    Iterator to the first output element.
/// @param fn     Unary operation applied to each element.
/// @return Iterator past the last written output element.
template <typename InIt, typename OutIt, typename Fn>
OutIt par_transform(InIt begin, InIt end, OutIt out, Fn fn) {
#if HPC_HAS_PARALLEL_STL
    return std::transform(std::execution::par_unseq, begin, end, out, fn);
#else
    return std::transform(begin, end, out, fn);
#endif
}

/// @brief Parallel reduce using `std::execution::par_unseq` when available.
///
/// Falls back to `std::accumulate` when parallel STL is not supported.
///
/// @tparam It    Input iterator type.
/// @tparam T     Value type of the accumulator.
/// @tparam BinOp Binary operation type (defaults to `std::plus<>`).
/// @param begin  Iterator to the first element.
/// @param end    Iterator past the last element.
/// @param init   Initial accumulator value.
/// @param op     Binary operation for reduction.
/// @return The reduced result.
template <typename It, typename T, typename BinOp = std::plus<>>
T par_reduce(It begin, It end, T init, BinOp op = {}) {
#if HPC_HAS_PARALLEL_STL
    return std::reduce(std::execution::par_unseq, begin, end, init, op);
#else
    return std::accumulate(begin, end, init, op);
#endif
}

/// @brief Parallel transform-reduce (fused map-reduce) using `std::execution::par_unseq`.
///
/// @tparam It          Input iterator type.
/// @tparam T           Value type of the accumulator.
/// @tparam ReduceOp    Binary reduction operation type.
/// @tparam TransformOp Unary transform operation type.
/// @param begin     Iterator to the first element.
/// @param end       Iterator past the last element.
/// @param init      Initial accumulator value.
/// @param reduce    Binary operation for reduction.
/// @param transform Unary operation applied before reduction.
/// @return The transform-reduced result.
template <typename It, typename T, typename ReduceOp, typename TransformOp>
T par_transform_reduce(It begin, It end, T init, ReduceOp reduce, TransformOp transform) {
#if HPC_HAS_PARALLEL_STL
    return std::transform_reduce(std::execution::par_unseq, begin, end, init, reduce, transform);
#else
    T acc = init;
    for (auto it = begin; it != end; ++it) acc = reduce(acc, transform(*it));
    return acc;
#endif
}

/// @brief Parallel for_each using `std::execution::par_unseq` when available.
///
/// @tparam It Input iterator type.
/// @tparam Fn Unary function object type.
/// @param begin Iterator to the first element.
/// @param end   Iterator past the last element.
/// @param fn    Unary operation applied to each element.
template <typename It, typename Fn>
void par_for_each(It begin, It end, Fn fn) {
#if HPC_HAS_PARALLEL_STL
    std::for_each(std::execution::par_unseq, begin, end, fn);
#else
    std::for_each(begin, end, fn);
#endif
}

} // namespace hpc
