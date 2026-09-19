#pragma once

#include "Nebrix/Core/Assert.h"
#include "Nebrix/ECS/ComponentPool.h"
#include "Nebrix/ECS/Entity.h"
#include "Nebrix/ECS/View.h"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nbx
{

    // Owns all entities and component pools of a game world.
    class World
    {
    public:
        World() = default;
        ~World();

        World(const World &) = delete;
        World &operator=(const World &) = delete;

        Entity createEntity();
        void destroyEntity(Entity entity);
        bool isAlive(Entity entity) const;
        size_t entityCount() const { return m_versions.size() - m_freeList.size(); }

        template <typename T>
        T &add(Entity entity, T component = {});

        template <typename T>
        bool has(Entity entity) const;

        template <typename T>
        T &get(Entity entity);

        template <typename T>
        void remove(Entity entity);

        template <typename T>
        ComponentPool<T> *pool();

        template <typename T>
        const ComponentPool<T> *pool() const;

        template <typename... Components>
        View<Components...> view();

    private:
        template <typename T>
        ComponentPool<T> &ensurePool();

    std::vector<uint32_t> m_versions;
    std::vector<uint8_t> m_alive; // byte per slot: vector<bool> is bit-packed and slow
    std::vector<uint32_t> m_freeList;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
    };

    // --- template implementations ---

    template <typename T>
    ComponentPool<T> &World::ensurePool()
    {
        const std::type_index key = typeid(T);
        std::unique_ptr<IComponentPool> &slot = m_pools[key];
        if (!slot)
            slot = std::make_unique<ComponentPool<T>>();
        return *static_cast<ComponentPool<T> *>(slot.get());
    }

    template <typename T>
    ComponentPool<T> *World::pool()
    {
        const auto it = m_pools.find(typeid(T));
        if (it == m_pools.end())
            return nullptr;
        return static_cast<ComponentPool<T> *>(it->second.get());
    }

    template <typename T>
    const ComponentPool<T> *World::pool() const
    {
        const auto it = m_pools.find(typeid(T));
        if (it == m_pools.end())
            return nullptr;
        return static_cast<const ComponentPool<T> *>(it->second.get());
    }

    template <typename T>
    T &World::add(Entity entity, T component)
    {
        NBX_ASSERT(isAlive(entity), "cannot add a component to a dead or stale entity");
        return ensurePool<T>().add(entity, std::move(component));
    }

    template <typename T>
    bool World::has(Entity entity) const
    {
        if (const ComponentPool<T> *p = pool<T>())
            return p->has(entity);
        return false;
    }

    template <typename T>
    T &World::get(Entity entity)
    {
        ComponentPool<T> *p = pool<T>();
        NBX_ASSERT(p != nullptr, "No component pool exists for this type");
        return p->get(entity);
    }

    template <typename T>
    void World::remove(Entity entity)
    {
        if (ComponentPool<T> *p = pool<T>())
            p->remove(entity);
    }

    template <typename... Components>
    View<Components...> World::view()
    {
        return View<Components...>(pool<Components>()...);
    }

} // namespace nbx