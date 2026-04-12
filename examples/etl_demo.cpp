/// @file etl_demo.cpp
/// @brief Demonstrates the ETL pipeline and parallel MapReduce utilities.

#include <etl/map_reduce.h>
#include <etl/pipeline.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

int main() {
    // ── 1. Lazy pipeline: filter → map → collect ────────────────────
    std::vector<int> numbers(100);
    std::iota(numbers.begin(), numbers.end(), 1);

    auto evens_squared = etl::from(numbers)
        .filter([](int x) { return x % 2 == 0; })
        .map([](int x) { return x * x; })
        .take(10)
        .collect();

    std::cout << "First 10 even squares: ";
    for (auto v : evens_squared) std::cout << v << ' ';
    std::cout << '\n';

    // ── 2. Pipeline reduce: sum of cubes of odd numbers ─────────────
    auto sum_odd_cubes = etl::from(numbers)
        .filter([](int x) { return x % 2 != 0; })
        .map([](int x) { return static_cast<long long>(x) * x * x; })
        .reduce(0LL, std::plus<>{});

    std::cout << "Sum of cubes of odd 1..100: " << sum_odd_cubes << '\n';

    // ── 3. FlatMap: split words from sentences ──────────────────────
    std::vector<std::string> sentences = {
        "hello world", "modern cpp", "etl pipeline"
    };

    auto words = etl::from(sentences)
        .flat_map([](const std::string& s) {
            std::vector<std::string> out;
            std::string tok;
            for (char c : s) {
                if (c == ' ') { if (!tok.empty()) { out.push_back(tok); tok.clear(); } }
                else tok += c;
            }
            if (!tok.empty()) out.push_back(tok);
            return out;
        })
        .collect();

    std::cout << "Words: ";
    for (auto& w : words) std::cout << '"' << w << "\" ";
    std::cout << '\n';

    // ── 4. Parallel MapReduce: sum of squares ───────────────────────
    std::vector<double> big(1'000'000);
    std::iota(big.begin(), big.end(), 1.0);

    auto t0 = std::chrono::steady_clock::now();
    double sum_sq = etl::map_reduce(big, 0.0,
        [](double x) { return x * x; },
        std::plus<>{});
    auto t1 = std::chrono::steady_clock::now();

    std::cout << "MapReduce sum of squares (1..1M): " << sum_sq
              << "  (" << std::chrono::duration<double, std::milli>(t1 - t0).count()
              << " ms)\n";

    // ── 5. Parallel map: compute sqrt of each element ───────────────
    auto roots = etl::parallel_map(big, [](double x) { return std::sqrt(x); });
    std::cout << "parallel_map sqrt: first=" << roots.front()
              << "  last=" << roots.back() << '\n';

    // ── 6. Pipeline count ───────────────────────────────────────────
    auto n_divisible_by_7 = etl::from(numbers)
        .filter([](int x) { return x % 7 == 0; })
        .count();
    std::cout << "Numbers 1..100 divisible by 7: " << n_divisible_by_7 << '\n';

    return 0;
}
