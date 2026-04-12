#pragma once
/// @file ecs.h
/// @brief Minimal Entity-Component-System using type-erased component storage.
///
/// Entities are lightweight IDs. Components are plain structs stored in typed
/// pools keyed by std::type_index. Systems iterate via World::each<Ts...>().

#include <any>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace simulation {

/// @brief Lightweight entity identifier (plain integer).
using Entity = std::uint32_t;

/// @brief Lightweight ECS world — owns entities and their components.
class World {
public:
    /// @brief Create a new entity with a unique ID.
    /// @return The newly created entity identifier.
    [[nodiscard]] Entity create() { return next_id_++; }

    /// @brief Attach a component to an entity, replacing any existing one of the same type.
    /// @tparam T Component type.
    /// @param e Target entity.
    /// @param component Component value to attach.
    template <typename T>
    void add(Entity e, T component) {
        pool<T>()[e] = std::move(component);
    }

    /// @brief Retrieve a pointer to an entity's component of type @p T.
    /// @tparam T Component type to look up.
    /// @param e Target entity.
    /// @return Pointer to the component, or `nullptr` if the entity lacks it.
    template <typename T>
    [[nodiscard]] T* get(Entity e) {
        auto& p = pool<T>();
        auto it = p.find(e);
        return it != p.end() ? &it->second : nullptr;
    }

    /// @brief Remove a component of type @p T from an entity.
    /// @tparam T Component type to remove.
    /// @param e Target entity.
    template <typename T>
    void remove(Entity e) { pool<T>().erase(e); }

    /// @brief Invoke @p fn for every entity that has all of @p Ts... components.
    /// @tparam Ts Component types required on each entity.
    /// @tparam Fn Callable with signature `void(Entity, Ts&...)`.
    /// @param fn Callback invoked for each matching entity.
    template <typename... Ts, typename Fn>
    void each(Fn&& fn) {
        // Iterate over the smallest pool for efficiency.
        auto& first = pool<first_t<Ts...>>();
        for (auto& [e, _] : first) {
            if (auto ptrs = try_get_all<Ts...>(e); ptrs) {
                std::apply([&](auto*... ps) { fn(e, *ps...); }, *ptrs);
            }
        }
    }

private:
    template <typename T, typename...>
    struct first { using type = T; };
    template <typename... Ts>
    using first_t = typename first<Ts...>::type;

    template <typename T>
    using Pool = std::unordered_map<Entity, T>;

    template <typename T>
    Pool<T>& pool() {
        auto idx = std::type_index(typeid(T));
        auto it = pools_.find(idx);
        if (it == pools_.end())
            it = pools_.emplace(idx, Pool<T>{}).first;
        return std::any_cast<Pool<T>&>(it->second);
    }

    template <typename... Ts>
    auto try_get_all(Entity e) -> std::optional<std::tuple<Ts*...>> {
        std::tuple<Ts*...> ptrs{get<Ts>(e)...};
        bool all = ((std::get<Ts*>(ptrs) != nullptr) && ...);
        if (!all) return std::nullopt;
        return ptrs;
    }

    Entity next_id_{0};
    std::unordered_map<std::type_index, std::any> pools_;
};

}  // namespace simulation
