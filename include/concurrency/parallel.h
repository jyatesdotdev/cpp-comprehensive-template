#pragma once

/// @file parallel.h
/// @brief Parallel algorithm helpers: parallel_for, async map-reduce, and
///        C++17 execution-policy wrappers.

#include <algorithm>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

// C++17 parallel STL (available with TBB or MSVC runtime)
#if __has_include(<execution>)
#include <execution>
#define HAS_PARALLEL_STL 1
#else
#define HAS_PARALLEL_STL 0
#endif

namespace concurrency {

/// @brief Partition [begin, end) across hardware threads and invoke @p fn on each element.
/// @tparam It Iterator type (must be random-access).
/// @tparam Fn Callable invoked as `fn(*it)` for each element.
/// @param begin     Start of the range.
/// @param end       End of the range.
/// @param fn        Function applied to every element.
/// @param n_threads Number of threads to use (0 = hardware concurrency).
template <typename It, typename Fn>
void parallel_for(It begin, It end, Fn fn, unsigned n_threads = 0) {
    if (begin == end) return;
    if (!n_threads) n_threads = std::thread::hardware_concurrency();
    const auto total = static_cast<std::size_t>(std::distance(begin, end));
    const auto chunk = (total + n_threads - 1) / n_threads;

    std::vector<std::future<void>> futs;
    for (unsigned t = 0; t < n_threads; ++t) {
        auto lo = begin + static_cast<std::ptrdiff_t>(std::min(t * chunk, total));
        auto hi = begin + static_cast<std::ptrdiff_t>(std::min((t + 1) * chunk, total));
        if (lo == hi) break;
        futs.push_back(std::async(std::launch::async, [lo, hi, &fn] {
            std::for_each(lo, hi, fn);
        }));
    }
    for (auto& f : futs) f.get(); // propagate exceptions
}

/// @brief Parallel map-reduce: apply @p map to each element, then combine with @p reduce.
/// @tparam It       Iterator type (must be random-access).
/// @tparam T        Accumulator / result type.
/// @tparam MapFn    Callable invoked as `map(*it)` returning a value convertible to @p T.
/// @tparam ReduceFn Binary callable invoked as `reduce(acc, mapped)` returning @p T.
/// @param begin     Start of the range.
/// @param end       End of the range.
/// @param init      Initial accumulator value.
/// @param map       Mapping function applied to each element.
/// @param reduce    Reduction function combining partial results.
/// @param n_threads Number of threads to use (0 = hardware concurrency).
/// @return The reduced result across all elements.
template <typename It, typename T, typename MapFn, typename ReduceFn>
T parallel_map_reduce(It begin, It end, T init, MapFn map, ReduceFn reduce,
                      unsigned n_threads = 0) {
    if (begin == end) return init;
    if (!n_threads) n_threads = std::thread::hardware_concurrency();
    const auto total = static_cast<std::size_t>(std::distance(begin, end));
    const auto chunk = (total + n_threads - 1) / n_threads;

    std::vector<std::future<T>> futs;
    for (unsigned t = 0; t < n_threads; ++t) {
        auto lo = begin + static_cast<std::ptrdiff_t>(std::min(t * chunk, total));
        auto hi = begin + static_cast<std::ptrdiff_t>(std::min((t + 1) * chunk, total));
        if (lo == hi) break;
        futs.push_back(std::async(std::launch::async, [lo, hi, init, &map, &reduce] {
            T acc = init;
            for (auto it = lo; it != hi; ++it) acc = reduce(acc, map(*it));
            return acc;
        }));
    }
    T result = init;
    for (auto& f : futs) result = reduce(result, f.get());
    return result;
}

} // namespace concurrency
