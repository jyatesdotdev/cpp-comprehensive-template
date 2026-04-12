/// @file patterns_tests.cpp
/// @brief Unit tests for Signal/Connection observer pattern.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include "patterns/observer.h"

using namespace patterns;

TEST_CASE("Signal connect and emit", "[patterns][observer]") {
    Signal<int> sig;
    int received = 0;
    auto conn = sig.connect([&](int v) { received = v; });

    sig.emit(42);
    REQUIRE(received == 42);
}

TEST_CASE("Signal multiple listeners", "[patterns][observer]") {
    Signal<int> sig;
    int sum = 0;
    auto c1 = sig.connect([&](int v) { sum += v; });
    auto c2 = sig.connect([&](int v) { sum += v * 10; });

    sig.emit(3);
    REQUIRE(sum == 33); // 3 + 30
}

TEST_CASE("Signal disconnect removes listener", "[patterns][observer]") {
    Signal<int> sig;
    int count = 0;
    auto conn = sig.connect([&](int) { ++count; });

    sig.emit(1);
    REQUIRE(count == 1);

    conn.disconnect();
    sig.emit(1);
    REQUIRE(count == 1); // not called again
    REQUIRE(sig.size() == 0);
}

TEST_CASE("Signal with no listeners is safe", "[patterns][observer]") {
    Signal<std::string> sig;
    REQUIRE_NOTHROW(sig.emit("hello"));
    REQUIRE(sig.size() == 0);
}
