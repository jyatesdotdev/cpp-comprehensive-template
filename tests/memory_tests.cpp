/// @file memory_tests.cpp
/// @brief Unit tests for Arena allocator, ArenaAllocator, and move semantics.

#include "memory/arena_allocator.h"
#include "memory/resource_handle.h"
#include "memory/smart_pointers.h"

#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <vector>

using namespace memory;

namespace {
struct CountingDel {
    inline static int closes = 0;
    void operator()(int) const noexcept { ++closes; }
};
} // namespace

TEST_CASE("Arena basic allocation", "[memory][arena]") {
    Arena arena(1024);
    REQUIRE(arena.capacity() == 1024);
    REQUIRE(arena.used() == 0);

    SECTION("single allocation bumps cursor") {
        auto *p = arena.allocate(64);
        REQUIRE(p != nullptr);
        REQUIRE(arena.used() >= 64);
    }

    SECTION("multiple allocations return distinct pointers") {
        auto *a = arena.allocate(32);
        auto *b = arena.allocate(32);
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
    REQUIRE_THROWS_AS(a.allocate(1), std::bad_alloc);
}

TEST_CASE("Arena rounds capacity up for aligned_alloc", "[memory][arena]") {
    Arena arena(1000);
    REQUIRE(arena.capacity() >= 1000);
    REQUIRE(arena.capacity() % alignof(std::max_align_t) == 0);
    REQUIRE(arena.allocate(16) != nullptr);
}

TEST_CASE("ArenaAllocator overflow throws", "[memory][arena]") {
    Arena arena(64);
    ArenaAllocator<int> alloc(arena);
    REQUIRE_THROWS_AS(alloc.allocate(std::numeric_limits<std::size_t>::max()), std::bad_alloc);
}

TEST_CASE("UniqueHandle releases on destroy", "[memory][handle]") {
    CountingDel::closes = 0;
    {
        UniqueHandle<int, CountingDel, -1> h(3);
        REQUIRE(static_cast<bool>(h));
        REQUIRE(h.get() == 3);
    }
    REQUIRE(CountingDel::closes == 1);
}

TEST_CASE("UniqueHandle move transfers ownership", "[memory][handle]") {
    CountingDel::closes = 0;
    UniqueHandle<int, CountingDel, -1> a(7);
    auto b = std::move(a);
    REQUIRE_FALSE(static_cast<bool>(a));
    REQUIRE(b.get() == 7);
    b.reset();
    REQUIRE(CountingDel::closes == 1);
    REQUIRE_FALSE(static_cast<bool>(b));
}

TEST_CASE("make_widget and make_c_buffer", "[memory][smart]") {
    auto w = make_widget(42);
    REQUIRE(w->value() == 42);
    auto buf = make_c_buffer(32);
    REQUIRE(static_cast<bool>(buf));
    auto empty = make_c_buffer(0);
    REQUIRE_FALSE(static_cast<bool>(empty));
}
