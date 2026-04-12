#pragma once

/// @file map_reduce.h
/// @brief Parallel MapReduce: partition data, map in parallel, then reduce.

#include <algorithm>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

namespace etl {

/// @brief Parallel map-reduce over a container.
///
/// 1. Partitions @p data across threads.
/// 2. Each thread applies @p map_fn to every element and locally reduces.
/// 3. Partial results are merged with @p reduce_fn.
///
/// @code
///   auto sum_sq = etl::map_reduce(vec, 0.0,
///       [](int x) { return x * x; },
///       std::plus<>{});
/// @endcode
///
/// @tparam Range    Container type with `size()`, `operator[]`, and `empty()`.
/// @tparam T        Accumulator / identity type.
/// @tparam MapFn    Unary callable (element) -> mapped value.
/// @tparam ReduceFn Binary callable (T, mapped) -> T.
/// @param data      Input data range.
/// @param identity  Identity element for the reduction.
/// @param map_fn    Mapping function applied to each element.
/// @param reduce_fn Reduction operator combining partial results.
/// @param n_threads Number of threads (0 = hardware concurrency).
/// @return The reduced result of type @p T.
template <typename Range, typename T, typename MapFn, typename ReduceFn>
T map_reduce(const Range& data, T identity, MapFn map_fn, ReduceFn reduce_fn,
             unsigned n_threads = 0) {
    if (data.empty()) return identity;
    if (!n_threads) n_threads = std::max(1u, std::thread::hardware_concurrency());

    const auto total = data.size();
    const auto chunk = (total + n_threads - 1) / n_threads;

    std::vector<std::future<T>> futures;
    for (unsigned t = 0; t < n_threads; ++t) {
        auto lo = std::min(t * chunk, total);
        auto hi = std::min(lo + chunk, total);
        if (lo == hi) break;
        futures.push_back(std::async(std::launch::async,
            [&data, lo, hi, identity, &map_fn, &reduce_fn] {
                T acc = identity;
                for (auto i = lo; i < hi; ++i)
                    acc = reduce_fn(acc, map_fn(data[i]));
                return acc;
            }));
    }

    T result = identity;
    for (auto& f : futures) result = reduce_fn(result, f.get());
    return result;
}

/// @brief Parallel map: apply @p fn to every element, returning a new vector.
/// @tparam Range Container type with `size()`, `operator[]`, and `empty()`.
/// @tparam Fn    Unary callable (element) -> mapped value.
/// @param data      Input data range.
/// @param fn        Mapping function applied to each element.
/// @param n_threads Number of threads (0 = hardware concurrency).
/// @return std::vector of mapped results preserving input order.
template <typename Range, typename Fn>
auto parallel_map(const Range& data, Fn fn, unsigned n_threads = 0) {
    using Out = std::invoke_result_t<Fn, typename Range::value_type>;
    if (data.empty()) return std::vector<Out>{};
    if (!n_threads) n_threads = std::max(1u, std::thread::hardware_concurrency());

    std::vector<Out> result(data.size());
    const auto total = data.size();
    const auto chunk = (total + n_threads - 1) / n_threads;

    std::vector<std::future<void>> futures;
    for (unsigned t = 0; t < n_threads; ++t) {
        auto lo = std::min(t * chunk, total);
        auto hi = std::min(lo + chunk, total);
        if (lo == hi) break;
        futures.push_back(std::async(std::launch::async,
            [&data, &result, lo, hi, &fn] {
                for (auto i = lo; i < hi; ++i)
                    result[i] = fn(data[i]);
            }));
    }
    for (auto& f : futures) f.get();
    return result;
}

} // namespace etl
