/// @file concurrency_tests.cpp
/// @brief Unit tests for SpscQueue single-threaded and cross-thread correctness.

#include <catch2/catch_test_macros.hpp>
#include <thread>
#include "concurrency/lock_free_queue.h"

using namespace concurrency;

TEST_CASE("SpscQueue single-threaded push/pop", "[concurrency][spsc]") {
    SpscQueue<int, 8> q;
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
        for (int i = 0; i < 7; ++i) // capacity 8 holds 7 elements (ring buffer)
            REQUIRE(q.push(i));
        REQUIRE_FALSE(q.push(99));
    }

    SECTION("FIFO ordering preserved") {
        q.push(1); q.push(2); q.push(3);
        REQUIRE(*q.pop() == 1);
        REQUIRE(*q.pop() == 2);
        REQUIRE(*q.pop() == 3);
    }
}

TEST_CASE("SpscQueue cross-thread correctness", "[concurrency][spsc]") {
    constexpr int N = 10'000;
    SpscQueue<int, 16384> q;

    std::thread producer([&] {
        for (int i = 0; i < N; ++i)
            while (!q.push(i)) std::this_thread::yield();
    });

    std::vector<int> received;
    received.reserve(N);
    std::thread consumer([&] {
        while (static_cast<int>(received.size()) < N) {
            if (auto v = q.pop()) received.push_back(*v);
            else std::this_thread::yield();
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(received.size() == N);
    for (int i = 0; i < N; ++i)
        REQUIRE(received[static_cast<std::size_t>(i)] == i);
}
