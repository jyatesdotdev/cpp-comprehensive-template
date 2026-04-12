# API Design Guide

Guidelines for designing clean, modern C++ APIs in this template.

## Core Principles

1. **Make interfaces hard to misuse** — prefer strong types over primitives
2. **Prefer value semantics** — pass by value when cheap, by `const&` otherwise
3. **Use RAII everywhere** — resources are managed by constructors/destructors
4. **Fail early and loudly** — use `std::expected`, exceptions, or `std::optional`

## Naming Conventions

```
namespace project::module   // lowercase, nested
class ThreadPool            // PascalCase for types
void process_batch()        // snake_case for functions
int max_threads_            // trailing underscore for members
constexpr int kBufferSize   // k-prefix for constants
```

## Function Signatures

```cpp
// Good: clear ownership, no ambiguity
auto create_widget(std::string name) -> std::unique_ptr<Widget>;
void process(std::span<const float> data);
auto find(int id) -> std::optional<Widget>;

// Avoid: raw pointers with unclear ownership
Widget* create_widget(const char* name);  // who owns this?
void process(float* data, size_t n);      // use span
```

## Ownership & Lifetime

| Signature | Meaning |
|---|---|
| `T` | Caller gives ownership (moved or copied) |
| `const T&` | Borrowed, read-only |
| `T&` | Borrowed, mutable |
| `std::unique_ptr<T>` | Transfer of exclusive ownership |
| `std::shared_ptr<T>` | Shared ownership (use sparingly) |
| `std::span<T>` | Non-owning view of contiguous data |
| `std::string_view` | Non-owning view of string data |

## Error Handling Strategy

```cpp
// Prefer std::expected (C++23) or std::optional for recoverable errors
auto parse_config(std::string_view path) -> std::expected<Config, Error>;

// Use exceptions for truly exceptional conditions
void connect(std::string_view host); // throws on network failure

// Use std::optional for "not found" semantics
auto find_user(int id) -> std::optional<User>;
```

## Template API Design

```cpp
// Constrain templates with concepts
template<std::invocable<int> F>
void for_each(std::span<int> data, F&& func);

// Provide deduction guides when useful
template<typename T>
class Wrapper {
public:
    explicit Wrapper(T val);
};
Wrapper(const char*) -> Wrapper<std::string>;
```

## Header Organization

```cpp
#pragma once

// 1. Standard library
#include <memory>
#include <string>

// 2. Third-party (conditional)
#ifdef HAS_FMT
#include <fmt/core.h>
#endif

// 3. Project headers
#include "core/app.h"

namespace project::module {

// Forward declarations first, then classes, then free functions

} // namespace project::module
```

## Namespace Policy

- Every module gets `namespace project::module_name`
- Implementation details go in `namespace detail`
- Never `using namespace` in headers
- ADL-friendly free functions live in the type's namespace

## ABI & Binary Compatibility

- Use the Pimpl idiom for stable ABI boundaries (see `core/app.h`)
- Prefer `enum class` over plain `enum`
- Mark single-argument constructors `explicit`
- Use `[[nodiscard]]` on functions where ignoring the return value is a bug

## Checklist for New APIs

- [ ] Does every resource-owning class follow Rule of Zero (or Five)?
- [ ] Are all single-argument constructors `explicit`?
- [ ] Are return values marked `[[nodiscard]]` where appropriate?
- [ ] Are templates constrained with concepts?
- [ ] Is the header self-contained (compiles on its own)?
- [ ] Are `string_view` / `span` used instead of raw pointer + size?

## See Also

- [Best Practices](best_practices.md) — Modern C++ coding guidelines
- [Architecture](ARCHITECTURE.md) — Module hierarchy and design patterns
- [Extending](EXTENDING.md) — Adding new libraries and modules
- [Third-Party Integration](third_party_integration.md) — Dependency management
