/// @file patterns_demo.cpp
/// @brief Demonstrates modern C++ design patterns

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "patterns/crtp.h"
#include "patterns/observer.h"
#include "patterns/type_erasure.h"
#include "patterns/visitor.h"

// ── Type-erasure: concrete types that satisfy the Drawable concept ───
/// @brief Concrete Drawable — a circle with a radius.
struct Circle {
    double radius;
    [[nodiscard]] std::string draw() const {
        return "Circle(r=" + std::to_string(radius) + ")";
    }
};
/// @brief Concrete Drawable — a square with a side length.
struct Square {
    double side;
    [[nodiscard]] std::string draw() const {
        return "Square(s=" + std::to_string(side) + ")";
    }
};

int main() {
    std::cout << "=== CRTP: Static Polymorphism ===\n";
    {
        patterns::Sensor a{"temp", 22.5};
        patterns::Sensor b{"humidity", 60.0};
        a.print();
        std::cout << "serialized: " << a.serialize() << '\n';
        std::cout << "a < b? " << (a < b ? "yes" : "no") << "\n\n";
    }

    std::cout << "=== Type Erasure: Drawable ===\n";
    {
        std::vector<patterns::Drawable> shapes;
        shapes.emplace_back(Circle{3.14});
        shapes.emplace_back(Square{5.0});
        for (const auto& s : shapes)
            std::cout << s.draw() << '\n';

        // Generic Function<>
        patterns::Function<int(int, int)> add = [](int a, int b) { return a + b; };
        std::cout << "Function<int(int,int)>(3,4) = " << add(3, 4) << "\n\n";
    }

    std::cout << "=== Observer: Signal/Slot ===\n";
    {
        patterns::Signal<std::string, int> on_event;
        int call_count = 0;

        auto c1 = on_event.connect([&](const std::string& name, int val) {
            std::cout << "  slot1: " << name << " = " << val << '\n';
            ++call_count;
        });
        {
            // c2 disconnects when sentinel goes out of scope
            auto c2 = on_event.connect([&](const std::string& name, int val) {
                std::cout << "  slot2: " << name << " = " << val << '\n';
                ++call_count;
            });
            on_event.emit("both_alive", 1);
            c2.disconnect();
        }
        on_event.emit("only_slot1", 2);
        std::cout << "total calls: " << call_count << "\n\n";
    }

    std::cout << "=== Visitor: Expression Tree ===\n";
    {
        // Build: -(3 + 4) * 2
        auto expr = patterns::bin('*',
            patterns::neg(patterns::bin('+', patterns::lit(3), patterns::lit(4))),
            patterns::lit(2));
        std::cout << "expr:  " << patterns::to_string(expr) << '\n';
        std::cout << "value: " << patterns::evaluate(expr) << '\n';
    }
}
