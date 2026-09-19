#pragma once

#include "Nebrix/ECS/ComponentPool.h"

#include <cstddef>
#include <tuple>
#include <utility>

namespace nbx
{

    // Iterates the entities that have ALL the requested components, over the dense
    // array of the first component type. Dereferencing yields (Entity, T&...).
    //
    // Perf note: cost is driven by the FIRST component's pool size (the others
    // are O(1) has() checks per entity), so order views rarest-component-first:
    // view<Player, Transform>, never view<Transform, Player>.
    template <typename... Components>
    class View
    {
    public:
        using Primary = std::tuple_element_t<0, std::tuple<Components...>>;

        explicit View(ComponentPool<Components> *...pools)
            : m_pools(std::make_tuple(pools...)) {}

        class Iterator
        {
        public:
            using value_type = std::tuple<Entity, Components &...>;

            Iterator(std::tuple<ComponentPool<Components> *...> pools, size_t index)
                : m_pools(pools), m_index(index)
            {
                skip();
            }

            value_type operator*() const
            {
                return deref(std::index_sequence_for<Components...>{});
            }

            Iterator &operator++()
            {
                ++m_index;
                skip();
                return *this;
            }

            bool operator!=(const Iterator &other) const { return m_index != other.m_index; }

        private:
            ComponentPool<Primary> *primary() const { return std::get<0>(m_pools); }

            void skip()
            {
                ComponentPool<Primary> *p = primary();
                while (p && m_index < p->size() && !hasAll())
                    ++m_index;
            }

            bool hasAll() const
            {
                return hasAllImpl(std::index_sequence_for<Components...>{});
            }

            template <size_t... Is>
            bool hasAllImpl(std::index_sequence<Is...>) const
            {
                const Entity entity = primary()->entityAt(m_index);
                bool all = true;
                ((all = all && std::get<Is>(m_pools) && std::get<Is>(m_pools)->has(entity)), ...);
                return all;
            }

            template <size_t... Is>
            value_type deref(std::index_sequence<Is...>) const
            {
                const Entity entity = primary()->entityAt(m_index);
                return value_type(entity, std::get<Is>(m_pools)->get(entity)...);
            }

            std::tuple<ComponentPool<Components> *...> m_pools;
            size_t m_index = 0;
        };

        Iterator begin() const { return Iterator(m_pools, 0); }

        Iterator end() const
        {
            ComponentPool<Primary> *p = std::get<0>(m_pools);
            return Iterator(m_pools, p ? p->size() : 0);
        }

        std::tuple<ComponentPool<Components> *...> m_pools;
    };

} // namespace nbx