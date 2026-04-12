/// @file memory_demo.cpp
/// @brief Demonstrates smart pointers, arena allocator, and RAII handles.

#include "memory/arena_allocator.h"
#include "memory/resource_handle.h"
#include "memory/smart_pointers.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    // ── 1. Smart pointer factory ────────────────────────────────────
    std::cout << "=== Smart Pointers ===\n";
    auto w = memory::make_widget(42);
    std::cout << "Widget value: " << w->value() << "\n";

    // Custom deleter for C memory
    auto buf = memory::make_c_buffer(256);
    std::cout << "C buffer allocated: " << (buf ? "yes" : "no") << "\n";

    // shared_from_this
    auto svc = std::make_shared<memory::SharedService>();
    svc->id = 7;
    auto ref = svc->get_ref();
    std::cout << "SharedService id via ref: " << ref->id
              << " (use_count=" << svc.use_count() << ")\n";

    // weak_ptr to break cycles
    std::weak_ptr<memory::SharedService> weak = svc;
    std::cout << "Weak expired? " << weak.expired() << "\n";
    svc.reset();
    ref.reset();
    std::cout << "After reset, weak expired? " << weak.expired() << "\n";

    // ── 2. Arena allocator ──────────────────────────────────────────
    std::cout << "\n=== Arena Allocator ===\n";
    memory::Arena arena(4096);
    {
        using Vec = std::vector<int, memory::ArenaAllocator<int>>;
        memory::ArenaAllocator<int> alloc{arena};
        Vec v(alloc);
        for (int i = 0; i < 10; ++i) v.push_back(i * i);
        std::cout << "Arena-backed vector:";
        for (auto x : v) std::cout << ' ' << x;
        std::cout << "\nArena used: " << arena.used() << " / " << arena.capacity() << "\n";
    }
    arena.reset();
    std::cout << "After reset: " << arena.used() << " bytes used\n";

    // ── 3. RAII handle ──────────────────────────────────────────────
    std::cout << "\n=== RAII Handle ===\n";
    {
        memory::UniqueFd fd; // default — invalid
        std::cout << "Default handle valid? " << static_cast<bool>(fd) << "\n";
        // In real code: UniqueFd fd(::open("file.txt", O_RDONLY));
    }
    std::cout << "Handle destroyed (auto-closed)\n";
}
