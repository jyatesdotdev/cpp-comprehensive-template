# Modern C++ Best Practices Guide

A practical reference for writing idiomatic, safe, and performant C++ using C++17/20/23 features.

---

## Table of Contents

1. [RAII and Resource Management](#raii-and-resource-management)
2. [Smart Pointers](#smart-pointers)
3. [Move Semantics and Value Categories](#move-semantics-and-value-categories)
4. [Modern Type System](#modern-type-system)
5. [C++17 Features](#c17-features)
6. [C++20 Features](#c20-features)
7. [C++23 Features](#c23-features)
8. [Error Handling](#error-handling)
9. [API Design Guidelines](#api-design-guidelines)
10. [Performance Guidelines](#performance-guidelines)

---

## RAII and Resource Management

RAII (Resource Acquisition Is Initialization) is the foundational C++ idiom: tie resource lifetime to object lifetime.

### Core Principle

```cpp
// BAD: Manual resource management
void bad() {
    auto* p = new Widget();
    do_work(p);   // if this throws, we leak
    delete p;
}

// GOOD: RAII — resource freed automatically
void good() {
    auto p = std::make_unique<Widget>();
    do_work(*p);  // exception-safe, no leak possible
}
```

### The Rule of Zero / Five

Prefer the **Rule of Zero**: if your class doesn't manage a raw resource directly, don't declare any special member functions. The compiler-generated defaults do the right thing.

```cpp
// Rule of Zero — preferred
struct Config {
    std::string name;
    std::vector<int> values;
    // No destructor, copy/move ctors, or assignment operators needed.
    // Compiler generates correct versions automatically.
};
```

When you must manage a resource (file handle, socket, GPU buffer), apply the **Rule of Five**:

```cpp
class FileHandle {
    int fd_ = -1;
public:
    explicit FileHandle(const char* path) : fd_(::open(path, O_RDONLY)) {}
    ~FileHandle() { if (fd_ >= 0) ::close(fd_); }

    FileHandle(const FileHandle&) = delete;              // non-copyable
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&& o) noexcept : fd_(std::exchange(o.fd_, -1)) {}
    FileHandle& operator=(FileHandle&& o) noexcept {
        if (this != &o) { if (fd_ >= 0) ::close(fd_); fd_ = std::exchange(o.fd_, -1); }
        return *this;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] explicit operator bool() const noexcept { return fd_ >= 0; }
};
```

### Pimpl Idiom

Use pimpl to hide implementation details and maintain ABI stability (see `core/app.h` in this template):

```cpp
// header — stable ABI, fast compilation
class Widget {
public:
    explicit Widget(std::string name);
    ~Widget();
    Widget(Widget&&) noexcept;
    Widget& operator=(Widget&&) noexcept;
    void draw() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

---

## Smart Pointers

### When to Use Which

| Pointer | Ownership | Use Case |
|---|---|---|
| `std::unique_ptr<T>` | Exclusive | Default choice. Single owner, zero overhead. |
| `std::shared_ptr<T>` | Shared | Multiple owners with shared lifetime. |
| `std::weak_ptr<T>` | Observing | Break cycles, optional observation of shared. |
| `T*` / `T&` | None | Non-owning access. Prefer references. |

### Key Patterns

```cpp
// Factory returning unique ownership
auto create_widget(std::string name) -> std::unique_ptr<Widget> {
    return std::make_unique<Widget>(std::move(name));
}

// Transfer ownership into a container
std::vector<std::unique_ptr<Widget>> widgets;
widgets.push_back(create_widget("btn"));

// Shared ownership — use sparingly
auto shared = std::make_shared<Config>();  // single allocation for object + control block

// Weak observation to break cycles
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> parent;  // prevents cycle
};

// Custom deleter for C APIs
auto file = std::unique_ptr<FILE, decltype(&fclose)>(fopen("data.bin", "rb"), &fclose);
```

### Guidelines

- **Default to `unique_ptr`**. Only use `shared_ptr` when shared ownership is genuinely required.
- **Never use `new`/`delete` directly**. Use `make_unique` / `make_shared`.
- **Pass smart pointers by value only to transfer or share ownership**. Otherwise pass `T&` or `T*`.
- **Use `std::observer_ptr<T>` (or raw `T*`)** for non-owning pointers when a reference won't work (nullable, rebindable).

---

## Move Semantics and Value Categories

### The Basics

Move semantics allow transferring resources instead of copying them. A moved-from object must be in a valid but unspecified state.

```cpp
std::vector<int> build_data() {
    std::vector<int> v(1'000'000);
    std::iota(v.begin(), v.end(), 0);
    return v;  // NRVO — no copy, no move (guaranteed in C++17)
}

void consume(std::vector<int> data) {  // takes by value
    // caller decides: copy or move
}

auto v = build_data();
consume(v);              // copies
consume(std::move(v));   // moves — v is now empty
```

### Forwarding References and Perfect Forwarding

```cpp
// Use auto&& or T&& with templates for forwarding references
template<typename... Args>
auto make_widget(Args&&... args) {
    return std::make_unique<Widget>(std::forward<Args>(args)...);
}
```

### When to Use `std::move`

- When passing a local variable you no longer need to a sink parameter.
- When returning a named local by move (only if NRVO can't apply — rare).
- **Never** `std::move` a `const` object (it silently copies).
- **Never** `std::move` the return value of a local variable (inhibits NRVO).

---

## Modern Type System

### `std::optional` — Nullable Values (C++17)

```cpp
auto find_user(int id) -> std::optional<User> {
    if (auto it = db.find(id); it != db.end())
        return *it;
    return std::nullopt;
}

if (auto user = find_user(42)) {
    std::cout << user->name;
}
```

### `std::variant` — Type-Safe Unions (C++17)

```cpp
using Value = std::variant<int, double, std::string>;

auto to_string(const Value& v) -> std::string {
    return std::visit([](const auto& val) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>)
            return val;
        else
            return std::to_string(val);
    }, v);
}
```

### `std::expected` — Error-or-Value (C++23)

```cpp
enum class ParseError { empty, overflow };

auto parse_int(std::string_view s) -> std::expected<int, ParseError> {
    if (s.empty()) return std::unexpected(ParseError::empty);
    // ... parse logic
    return result;
}

// Monadic chaining (C++23)
auto doubled = parse_int("42").transform([](int v) { return v * 2; });
```

---

## C++17 Features

### Structured Bindings

```cpp
auto [name, age] = get_person();

for (const auto& [key, value] : my_map) {
    std::cout << key << ": " << value << "\n";
}
```

### `if constexpr` — Compile-Time Branching

```cpp
template<typename T>
auto serialize(const T& val) -> std::string {
    if constexpr (std::is_arithmetic_v<T>)
        return std::to_string(val);
    else if constexpr (std::is_same_v<T, std::string>)
        return val;
    else
        return val.to_string();  // requires member
}
```

### `std::string_view` — Non-Owning String Reference

```cpp
// Zero-copy string parameter — use instead of const std::string&
void log(std::string_view msg) {
    std::cout << msg;
}

log("literal");           // no allocation
log(some_string);         // no copy
log(some_string.substr(0, 10));  // WARNING: string_view::substr is fine,
                                  // but don't store views to temporaries
```

### Class Template Argument Deduction (CTAD)

```cpp
std::pair p{1, 3.14};           // std::pair<int, double>
std::vector v{1, 2, 3};         // std::vector<int>
std::lock_guard lock{mutex};     // deduces mutex type
```

### `std::filesystem`

```cpp
namespace fs = std::filesystem;

for (auto& entry : fs::recursive_directory_iterator("src/")) {
    if (entry.path().extension() == ".cpp")
        std::cout << entry.path() << " (" << fs::file_size(entry) << " bytes)\n";
}
```

---

## C++20 Features

### Concepts — Constrained Templates

```cpp
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<Numeric T>
auto square(T x) -> T { return x * x; }

// Terse syntax
auto cube(Numeric auto x) { return x * x * x; }
```

### Ranges — Composable Algorithms

```cpp
#include <ranges>
namespace rv = std::views;

auto evens_squared = std::views::iota(1, 100)
    | rv::filter([](int n) { return n % 2 == 0; })
    | rv::transform([](int n) { return n * n; })
    | rv::take(10);

for (int v : evens_squared) std::cout << v << " ";
```

### Coroutines

```cpp
#include <coroutine>

// Generator pattern (simplified — full impl requires promise_type)
template<typename T>
struct Generator {
    struct promise_type { /* ... */ };
    // yields values lazily
};

Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}
```

### Three-Way Comparison (`<=>`)

```cpp
struct Point {
    int x, y;
    auto operator<=>(const Point&) const = default;  // generates all 6 operators
};
```

### `std::format`

```cpp
auto msg = std::format("Hello, {}! You have {} items.", name, count);
// Type-safe, no buffer overflows, extensible via std::formatter
```

### Modules (C++20, compiler support varies)

```cpp
// math.cppm
export module math;
export auto add(int a, int b) -> int { return a + b; }

// main.cpp
import math;
int main() { return add(1, 2); }
```

---

## C++23 Features

### `std::print` — Type-Safe I/O

```cpp
std::print("Hello, {}!\n", name);  // replaces printf and iostream
std::println("Value: {}", 42);     // with newline
```

### Deducing `this` — Explicit Object Parameter

```cpp
struct Widget {
    // One function handles both const and non-const
    template<typename Self>
    auto& value(this Self&& self) { return self.value_; }
private:
    int value_;
};
```

### `std::mdspan` — Multidimensional View

```cpp
std::vector<double> data(rows * cols);
auto matrix = std::mdspan(data.data(), rows, cols);
matrix[i, j] = 3.14;  // multidimensional subscript (C++23)
```

### `std::generator` — Standard Coroutine Generator

```cpp
std::generator<int> range(int start, int end) {
    for (int i = start; i < end; ++i)
        co_yield i;
}
```

---

## Error Handling

### Strategy Selection

| Situation | Mechanism |
|---|---|
| Programming errors (bugs) | `assert`, contracts (C++26), `[[assume]]` |
| Recoverable runtime errors | `std::expected<T,E>` (C++23) or exceptions |
| Truly exceptional conditions | Exceptions |
| Performance-critical hot paths | Error codes or `std::expected` |

### Exception Safety Guarantees

1. **No-throw**: Operation cannot fail. Mark with `noexcept`.
2. **Strong**: If it fails, state is rolled back (as if nothing happened).
3. **Basic**: If it fails, invariants are maintained but state may change.

```cpp
// Strong guarantee via copy-and-swap
class Container {
    void swap(Container& other) noexcept;
public:
    Container& operator=(Container other) {  // copy by value
        swap(other);                          // noexcept swap
        return *this;                         // old data destroyed with 'other'
    }
};
```

### `noexcept` Guidelines

- Mark move constructors/assignment `noexcept` — enables optimizations in STL containers.
- Mark swap, destructors, and simple getters `noexcept`.
- Don't mark functions `noexcept` unless you're certain they won't throw.

---

## API Design Guidelines

### Parameter Passing

```
┌─────────────────────────────────────────────────────────┐
│ Cheap to copy (int, double, ptr)?  → Pass by value      │
│ Read-only access?                  → const T&            │
│ String-like read-only?             → std::string_view    │
│ Span of elements read-only?        → std::span<const T>  │
│ Sink parameter (will store)?       → T by value + move   │
│ Transfer ownership?                → std::unique_ptr<T>  │
│ Share ownership?                   → std::shared_ptr<T>  │
│ Optional/nullable non-owning?      → T* (raw pointer)    │
└─────────────────────────────────────────────────────────┘
```

### Return Types

- Return by value (NRVO/move makes this efficient).
- Use `std::optional<T>` for "might not have a result."
- Use `std::expected<T, E>` for operations that can fail with error info.
- Return `std::unique_ptr<T>` for factory functions.

### Const Correctness

```cpp
class Cache {
    mutable std::mutex mtx_;           // mutable for logical const
    std::unordered_map<int, Data> data_;
public:
    // const method — thread-safe read
    auto get(int key) const -> std::optional<Data> {
        std::lock_guard lock{mtx_};
        if (auto it = data_.find(key); it != data_.end())
            return it->second;
        return std::nullopt;
    }
};
```

---

## Performance Guidelines

### Avoid Unnecessary Copies

```cpp
// BAD: copies the string
for (auto s : string_vector) { ... }

// GOOD: const reference
for (const auto& s : string_vector) { ... }

// GOOD: string_view for read-only string params
void process(std::string_view data);
```

### Reserve Containers

```cpp
std::vector<Widget> widgets;
widgets.reserve(expected_count);  // one allocation instead of many
```

### Use `emplace` Over `push`

```cpp
widgets.emplace_back("name", 42);  // constructs in-place
// vs
widgets.push_back(Widget{"name", 42});  // constructs then moves
```

### Prefer Stack Allocation

```cpp
// BAD: heap allocation for small object
auto p = std::make_unique<Point>(1, 2);

// GOOD: stack allocation
Point p{1, 2};
```

### `[[nodiscard]]` — Prevent Ignored Returns

```cpp
[[nodiscard]] auto compute() -> Result;
// Compiler warns if return value is discarded
```

### `[[likely]]` / `[[unlikely]]` — Branch Hints (C++20)

```cpp
if (cache.contains(key)) [[likely]] {
    return cache[key];
} else [[unlikely]] {
    return expensive_compute(key);
}
```

---

## Quick Reference: What Standard Introduced What

| Feature | Standard |
|---|---|
| Smart pointers, move semantics, `auto`, lambdas, `constexpr` | C++11 |
| Generic lambdas, `make_unique`, variable templates | C++14 |
| `optional`, `variant`, `string_view`, structured bindings, `if constexpr`, `filesystem`, fold expressions | C++17 |
| Concepts, ranges, coroutines, `<=>`, `std::format`, modules, `consteval`, `constinit` | C++20 |
| `expected`, `print`, deducing `this`, `mdspan`, `generator`, `std::flat_map` | C++23 |

---

*This guide is a living document. Update it as new patterns emerge and compiler support evolves.*

## See Also

- [API Design](api_design.md) — C++ API design patterns
- [Architecture](ARCHITECTURE.md) — Project structure and design patterns used
- [Toolchain](TOOLCHAIN.md) — Compiler versions and C++20 feature support
- [HPC Optimization](hpc_optimization.md) — Performance optimization techniques
