/// @file concurrency_tests.cpp
/// @brief Unit tests for SpscQueue single-threaded and cross-thread correctness.

#include "concurrency/lock_free_queue.h"
#include "concurrency/parallel.h"
#include "concurrency/thread_pool.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <future>
#include <numeric>
#include <string>
#include <thread>
#include <utility>
#include <vector>

TEST_CASE("SpscQueue single-threaded push/pop", "[concurrency][spsc]") {
    concurrency::SpscQueue<int, 8> q;
    REQUIRE(q.empty());

    SECTION("push and pop single element") {
        REQUIRE(q.push(42));
        auto val = q.pop();
        REQUIRE(val.has_value());
        REQUIRE(*val == 42);
        REQUIRE(q.empty());
    }

    SECTION("pop from empty returns nullopt") {
        REQUIRE_FALSE(q.pop().has_value());
    }

    SECTION("push to full returns false") {
        REQUIRE(concurrency::SpscQueue<int, 8>::capacity() == 7);
        for (int i = 0; i < 7; ++i) // capacity 8 holds 7 elements (ring buffer)
            REQUIRE(q.push(i));
        REQUIRE_FALSE(q.push(99));
    }

    SECTION("FIFO ordering preserved") {
        q.push(1);
        q.push(2);
        q.push(3);
        REQUIRE(*q.pop() == 1);
        REQUIRE(*q.pop() == 2);
        REQUIRE(*q.pop() == 3);
    }
}

TEST_CASE("SpscQueue push rvalue overload", "[concurrency][spsc]") {
    concurrency::SpscQueue<std::string, 4> q;
    std::string s = "hello";
    REQUIRE(q.push(std::move(s)));
    auto val = q.pop();
    REQUIRE(val.has_value());
    REQUIRE(*val == "hello");
}

TEST_CASE("SpscQueue cross-thread correctness", "[concurrency][spsc]") {
    constexpr int N = 10'000;
    concurrency::SpscQueue<int, 16384> q;

    std::thread producer([&] {
        for (int i = 0; i < N; ++i)
            while (!q.push(i))
                std::this_thread::yield();
    });

    std::vector<int> received;
    received.reserve(N);
    std::thread consumer([&] {
        while (static_cast<int>(received.size()) < N) {
            if (auto v = q.pop())
                received.push_back(*v);
            else
                std::this_thread::yield();
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(received.size() == N);
    for (int i = 0; i < N; ++i)
        REQUIRE(received[static_cast<std::size_t>(i)] == i);
}

TEST_CASE("ThreadPool executes work and drains on destroy", "[concurrency][pool]") {
    std::future<int> f;
    {
        concurrency::ThreadPool pool{2};
        REQUIRE(pool.size() == 2);
        f = pool.submit([] { return 41 + 1; });
        auto g = pool.submit([] { return 7; });
        REQUIRE(g.get() == 7);
    }
    REQUIRE(f.get() == 42);
}

TEST_CASE("parallel_for visits every element", "[concurrency][parallel]") {
    std::vector<int> v(128, 0);
    concurrency::parallel_for(v.begin(), v.end(), [](int &x) { x = 1; });
    REQUIRE(std::all_of(v.begin(), v.end(), [](int x) { return x == 1; }));
}

TEST_CASE("parallel_map_reduce sums squares", "[concurrency][parallel]") {
    std::vector<int> v(32);
    std::iota(v.begin(), v.end(), 1);
    auto sum = concurrency::parallel_map_reduce(
        v.begin(), v.end(), 0, [](int x) { return x * x; }, std::plus<>{});
    REQUIRE(sum == 32 * 33 * 65 / 6); // n(n+1)(2n+1)/6
}
