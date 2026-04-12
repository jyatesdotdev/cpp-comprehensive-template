/// @file memory_tests.cpp
/// @brief Unit tests for Arena allocator, ArenaAllocator, and move semantics.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include "memory/arena_allocator.h"

using namespace memory;

TEST_CASE("Arena basic allocation", "[memory][arena]") {
    Arena arena(1024);
    REQUIRE(arena.capacity() == 1024);
    REQUIRE(arena.used() == 0);

    SECTION("single allocation bumps cursor") {
        auto* p = arena.allocate(64);
        REQUIRE(p != nullptr);
        REQUIRE(arena.used() >= 64);
    }

    SECTION("multiple allocations return distinct pointers") {
        auto* a = arena.allocate(32);
        auto* b = arena.allocate(32);
        REQUIRE(a != b);
    }

    SECTION("reset reclaims all memory") {
        arena.allocate(512);
        arena.reset();
        REQUIRE(arena.used() == 0);
    }

    SECTION("overflow throws bad_alloc") {
        REQUIRE_THROWS_AS(arena.allocate(2048), std::bad_alloc);
    }
}

TEST_CASE("ArenaAllocator works with std::vector", "[memory][arena]") {
    Arena arena(4096);
    ArenaAllocator<int> alloc(arena);
    std::vector<int, ArenaAllocator<int>> vec(alloc);

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    REQUIRE(vec.size() == 3);
    REQUIRE(vec[0] == 1);
    REQUIRE(arena.used() > 0);
}

TEST_CASE("Arena move constructor transfers ownership", "[memory][arena]") {
    Arena a(512);
    a.allocate(100);
    auto used = a.used();

    Arena b(std::move(a));
    REQUIRE(b.used() == used);
    REQUIRE(b.capacity() == 512);
}
