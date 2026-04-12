/// @file concurrency_demo.cpp
/// @brief Demonstrates thread pool, lock-free SPSC queue, and parallel algorithms.

#include "concurrency/lock_free_queue.h"
#include "concurrency/parallel.h"
#include "concurrency/thread_pool.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

int main() {
    // ── 1. Thread Pool ──────────────────────────────────────────────
    std::cout << "=== Thread Pool ===\n";
    {
        concurrency::ThreadPool pool{4};
        std::vector<std::future<double>> futs;
        for (int i = 1; i <= 8; ++i) {
            futs.push_back(pool.submit([i] { return std::sqrt(static_cast<double>(i)); }));
        }
        for (int i = 0; auto& f : futs) {
            std::cout << "sqrt(" << ++i << ") = " << f.get() << "\n";
        }
    }

    // ── 2. Lock-Free SPSC Queue ─────────────────────────────────────
    std::cout << "\n=== SPSC Queue ===\n";
    {
        concurrency::SpscQueue<int, 256> q;

        // Producer thread
        std::jthread producer([&q] {
            for (int i = 0; i < 100; ++i) {
                while (!q.push(i)) {} // spin until space available
            }
        });

        // Consumer (this thread)
        int sum = 0, count = 0;
        while (count < 100) {
            if (auto val = q.pop()) {
                sum += *val;
                ++count;
            }
        }
        std::cout << "Received " << count << " items, sum = " << sum << "\n";
        // Expected: sum of 0..99 = 4950
    }

    // ── 3. Parallel For ─────────────────────────────────────────────
    std::cout << "\n=== Parallel For ===\n";
    {
        std::vector<double> data(1'000'000, 1.0);
        concurrency::parallel_for(data.begin(), data.end(),
                                  [](double& x) { x = std::sin(x) * std::cos(x); });
        std::cout << "Processed " << data.size() << " elements in parallel\n";
        std::cout << "Sample data[0] = " << data[0] << "\n";
    }

    // ── 4. Parallel Map-Reduce ──────────────────────────────────────
    std::cout << "\n=== Parallel Map-Reduce ===\n";
    {
        std::vector<int> nums(1'000'000);
        std::iota(nums.begin(), nums.end(), 1);

        auto sum = concurrency::parallel_map_reduce(
            nums.begin(), nums.end(), 0L,
            [](int x) -> long { return static_cast<long>(x) * x; }, // map: square
            std::plus<long>{}                                        // reduce: sum
        );
        std::cout << "Sum of squares 1.." << nums.size() << " = " << sum << "\n";
    }

    // ── 5. std::async quick example ─────────────────────────────────
    std::cout << "\n=== std::async ===\n";
    {
        auto fut = std::async(std::launch::async, [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return 42;
        });
        std::cout << "Async result: " << fut.get() << "\n";
    }
}
