#pragma once

/// @file pipeline.h
/// @brief Composable, lazy ETL pipeline with fluent API.
///
/// Stages are chained at compile time via templates. Data flows only when a
/// terminal operation (collect, reduce, for_each, count) is invoked.

#include <algorithm>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

namespace etl {

// ── Stage tags ──────────────────────────────────────────────────────

/// @brief Identity source — wraps a range and yields its elements.
/// @tparam Range Container type that supports range-based iteration.
template <typename Range>
struct Source {
    const Range& data; ///< Reference to the source data range.
    /// @brief Construct a Source from a range.
    /// @param d The data range to wrap.
    explicit Source(const Range& d) : data(d) {}
};

/// @brief Map stage — transforms each element.
/// @tparam Prev Previous pipeline stage type.
/// @tparam Fn Unary callable applied to each element.
template <typename Prev, typename Fn>
struct Map {
    Prev prev; ///< Previous stage.
    Fn fn;     ///< Transformation function.
};

/// @brief Filter stage — keeps elements satisfying a predicate.
/// @tparam Prev Previous pipeline stage type.
/// @tparam Pred Unary predicate returning bool.
template <typename Prev, typename Pred>
struct Filter {
    Prev prev;  ///< Previous stage.
    Pred pred;  ///< Filter predicate.
};

/// @brief FlatMap stage — maps each element to a range and flattens.
/// @tparam Prev Previous pipeline stage type.
/// @tparam Fn Callable returning a container for each element.
template <typename Prev, typename Fn>
struct FlatMap {
    Prev prev; ///< Previous stage.
    Fn fn;     ///< Flat-mapping function.
};

/// @brief Take stage — yields at most N elements.
/// @tparam Prev Previous pipeline stage type.
template <typename Prev>
struct Take {
    Prev prev;      ///< Previous stage.
    std::size_t n;  ///< Maximum number of elements to yield.
};

// ── Element-type trait (portable across GCC / Clang) ────────────────

/// @brief Deduces the element type produced by a pipeline stage.
template <typename S> struct element_type;

template <typename R>
struct element_type<Source<R>> { using type = typename R::value_type; };

template <typename P, typename Fn>
struct element_type<Map<P, Fn>> {
    using type = std::invoke_result_t<Fn, typename element_type<P>::type>;
};

template <typename P, typename Pred>
struct element_type<Filter<P, Pred>> { using type = typename element_type<P>::type; };

template <typename P, typename Fn>
struct element_type<FlatMap<P, Fn>> {
    using type = typename std::invoke_result_t<Fn, typename element_type<P>::type>::value_type;
};

template <typename P>
struct element_type<Take<P>> { using type = typename element_type<P>::type; };

template <typename S>
using element_type_t = typename element_type<S>::type;

// ── Pipeline wrapper (provides fluent API) ──────────────────────────

/// @brief Composable lazy pipeline with a fluent API.
/// @tparam Stage The current pipeline stage type.
///
/// Stages are chained at compile time. Data flows only when a terminal
/// operation (collect, reduce, for_each, count) is invoked.
template <typename Stage>
class Pipeline {
public:
    /// @brief Construct a pipeline from a stage.
    /// @param s The initial or composed stage.
    explicit Pipeline(Stage s) : stage_(std::move(s)) {}

    /// @brief Transform each element.
    /// @tparam Fn Unary callable.
    /// @param fn Transformation function.
    /// @return A new Pipeline with a Map stage appended.
    template <typename Fn>
    auto map(Fn fn) const {
        return Pipeline<Map<Stage, Fn>>{Map<Stage, Fn>{stage_, std::move(fn)}};
    }

    /// @brief Keep elements where @p pred returns true.
    /// @tparam Pred Unary predicate.
    /// @param pred Filter predicate.
    /// @return A new Pipeline with a Filter stage appended.
    template <typename Pred>
    auto filter(Pred pred) const {
        return Pipeline<Filter<Stage, Pred>>{Filter<Stage, Pred>{stage_, std::move(pred)}};
    }

    /// @brief Map each element to a container and flatten results.
    /// @tparam Fn Callable returning a container.
    /// @param fn Flat-mapping function.
    /// @return A new Pipeline with a FlatMap stage appended.
    template <typename Fn>
    auto flat_map(Fn fn) const {
        return Pipeline<FlatMap<Stage, Fn>>{FlatMap<Stage, Fn>{stage_, std::move(fn)}};
    }

    /// @brief Limit output to at most @p n elements.
    /// @param n Maximum number of elements to yield.
    /// @return A new Pipeline with a Take stage appended.
    auto take(std::size_t n) const {
        return Pipeline<Take<Stage>>{Take<Stage>{stage_, n}};
    }

    // ── Terminal operations ─────────────────────────────────────────

    /// @brief Collect results into a vector.
    /// @tparam Out (deduced) Element type of the output vector.
    /// @return std::vector containing all pipeline output elements.
    template <typename Out = void>
    auto collect() const {
        using elem_t = element_type_t<Stage>;
        std::vector<elem_t> out;
        evaluate(stage_, [&](auto&& v) { out.push_back(std::forward<decltype(v)>(v)); });
        return out;
    }

    /// @brief Fold all elements with an accumulator.
    /// @tparam T Accumulator type.
    /// @tparam BinOp Binary callable (T, element) -> T.
    /// @param init Initial accumulator value.
    /// @param op Binary reduction operator.
    /// @return The final accumulated value.
    template <typename T, typename BinOp>
    T reduce(T init, BinOp op) const {
        evaluate(stage_, [&](auto&& v) { init = op(std::move(init), std::forward<decltype(v)>(v)); });
        return init;
    }

    /// @brief Apply a function to every element (side-effect terminal).
    /// @tparam Fn Unary callable.
    /// @param fn Function applied to each element.
    template <typename Fn>
    void for_each(Fn fn) const {
        evaluate(stage_, fn);
    }

    /// @brief Count elements that pass through the pipeline.
    /// @return Number of elements.
    std::size_t count() const {
        std::size_t n = 0;
        evaluate(stage_, [&](auto&&) { ++n; });
        return n;
    }

    /// @brief Access the underlying stage.
    /// @return Const reference to the current stage.
    const Stage& stage() const { return stage_; }

private:
    Stage stage_;

    // ── Lazy evaluation engine (recursive over stage types) ─────────

    // ── evaluate: push each element through stages into a callback ──

    template <typename R, typename Cb>
    static void evaluate(const Source<R>& s, Cb&& cb) {
        for (auto&& e : s.data) cb(e);
    }

    template <typename P, typename Fn, typename Cb>
    static void evaluate(const Map<P, Fn>& s, Cb&& cb) {
        evaluate(s.prev, [&](auto&& v) { cb(s.fn(std::forward<decltype(v)>(v))); });
    }

    template <typename P, typename Pred, typename Cb>
    static void evaluate(const Filter<P, Pred>& s, Cb&& cb) {
        evaluate(s.prev, [&](auto&& v) {
            if (s.pred(v)) cb(std::forward<decltype(v)>(v));
        });
    }

    template <typename P, typename Fn, typename Cb>
    static void evaluate(const FlatMap<P, Fn>& s, Cb&& cb) {
        evaluate(s.prev, [&](auto&& v) {
            for (auto&& inner : s.fn(std::forward<decltype(v)>(v))) cb(inner);
        });
    }

    template <typename P, typename Cb>
    static void evaluate(const Take<P>& s, Cb&& cb) {
        std::size_t remaining = s.n;
        try {
            evaluate(s.prev, [&](auto&& v) {
                if (remaining == 0) throw StopIteration{};
                --remaining;
                cb(std::forward<decltype(v)>(v));
            });
        } catch (const StopIteration&) {}
    }

    struct StopIteration {};
};

// ── Factory: start a pipeline from any range ────────────────────────

/// @brief Create a lazy pipeline from a container or range.
/// @tparam Range Container type supporting range-based iteration.
/// @param data The source data range.
/// @return A Pipeline wrapping a Source stage over @p data.
template <typename Range>
auto from(const Range& data) {
    return Pipeline<Source<Range>>{Source<Range>{data}};
}

} // namespace etl
