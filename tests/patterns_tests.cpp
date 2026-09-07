/// @file patterns_tests.cpp
/// @brief Unit tests for Signal/Connection observer pattern.

#include "patterns/crtp.h"
#include "patterns/observer.h"
#include "patterns/type_erasure.h"
#include "patterns/visitor.h"

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>

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

TEST_CASE("ScopedConnection disconnects on destruction", "[patterns][observer]") {
    Signal<int> sig;
    int count = 0;
    {
        ScopedConnection sc{sig.connect([&](int) { ++count; })};
        sig.emit(1);
        REQUIRE(count == 1);
    }
    sig.emit(1);
    REQUIRE(count == 1);
}

TEST_CASE("Sensor CRTP serialize round-trip", "[patterns][crtp]") {
    Sensor s{"temp", 21.5};
    auto encoded = s.serialize();
    Sensor t;
    t.deserialize(encoded);
    REQUIRE(t.name == "temp");
    REQUIRE_THROWS_AS(t.deserialize("no-separator"), std::invalid_argument);
}

TEST_CASE("Drawable type erasure", "[patterns][erasure]") {
    struct Circle {
        std::string draw() const { return "circle"; }
    };
    Drawable d{Circle{}};
    REQUIRE(d.draw() == "circle");
}

TEST_CASE("Function const invoke", "[patterns][erasure]") {
    const Function<int(int)> f{[](int x) { return x + 1; }};
    REQUIRE(f(41) == 42);
}

TEST_CASE("visitor evaluates expression trees", "[patterns][visitor]") {
    auto expr = bin('+', lit(2.0), bin('*', lit(3.0), lit(4.0)));
    REQUIRE(evaluate(expr) == 14.0);
    REQUIRE(evaluate(neg(lit(5.0))) == -5.0);
    REQUIRE_THROWS_AS(evaluate(bin('/', lit(1.0), lit(0.0))), std::invalid_argument);
}
