#pragma once

#include "Nebrix/Core/Assert.h"
#include "Nebrix/ECS/Entity.h"

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace nbx
{

    // Type-erased base so World can own pools of any component type.
    class IComponentPool
    {
    public:
        virtual ~IComponentPool() = default;
        virtual void onEntityDestroyed(uint32_t entityIndex) = 0;
    };

    // Sparse-set storage for one component type T: dense arrays (components +
    // entities) for cache-friendly iteration, plus a sparse index (entity -> dense).
    // Removal is swap-and-pop, so iteration order is unstable.
    template <typename T>
    class ComponentPool final : public IComponentPool
    {
    public:
        struct SparseEntry
        {
            uint32_t dense = std::numeric_limits<uint32_t>::max();
            uint32_t generation = 0;
        };

        T &add(const Entity &entity, T component = {})
        {
            const uint32_t dense = getDense(entity);
            if (dense != kInvalid)
            {
                m_components[dense] = std::move(component);
                return m_components[dense];
            }
            m_entities.push_back(entity);
            m_components.push_back(std::move(component));
            setSparse(entity, static_cast<uint32_t>(m_entities.size() - 1));
            return m_components.back();
        }

        void remove(const Entity &entity)
        {
            const uint32_t dense = getDense(entity);
            if (dense == kInvalid)
                return;
            const uint32_t generation = m_sparse[entity.index].generation;
            eraseDense(dense);
            // Keep the stored generation: the entry becomes a tombstone that
            // still documents which incarnation of this slot held a component.
            m_sparse[entity.index] = {kInvalid, generation};
        }

        bool has(const Entity &entity) const { return getDense(entity) != kInvalid; }

        T &get(const Entity &entity)
        {
            const uint32_t dense = getDense(entity);
            NBX_ASSERT(dense != kInvalid, "Entity does not have this component");
            return m_components[dense];
        }

        uint32_t size() const { return static_cast<uint32_t>(m_components.size()); }

        Entity entityAt(uint32_t dense) const { return m_entities[dense]; }

        void onEntityDestroyed(uint32_t entityIndex) override
        {
            if (entityIndex >= m_sparse.size())
                return;
            const uint32_t dense = m_sparse[entityIndex].dense;
            if (dense == kInvalid)
                return;
            const uint32_t generation = m_sparse[entityIndex].generation;
            eraseDense(dense);
            m_sparse[entityIndex] = {kInvalid, generation};
        }

    private:
        static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

        uint32_t getDense(const Entity &entity) const
        {
            if (entity.index >= m_sparse.size())
                return kInvalid;
            const SparseEntry &entry = m_sparse[entity.index];
            if (entry.dense == kInvalid || entry.generation != entity.generation)
                return kInvalid;
            return entry.dense;
        }

        void setSparse(const Entity &entity, uint32_t dense)
        {
            if (entity.index >= m_sparse.size())
                m_sparse.resize(static_cast<size_t>(entity.index) + 1);
            m_sparse[entity.index] = {dense, entity.generation};
        }

        // Swap-and-pop: moves the last dense element into the freed slot.
        void eraseDense(uint32_t dense)
        {
            NBX_ASSERT(!m_components.empty(), "eraseDense on an empty pool");
            if (m_components.empty())
                return;
            const uint32_t last = static_cast<uint32_t>(m_components.size() - 1);
            if (dense != last)
            {
                m_entities[dense] = m_entities[last];
                m_components[dense] = std::move(m_components[last]);
                m_sparse[m_entities[dense].index].dense = dense;
            }
            m_entities.pop_back();
            m_components.pop_back();
        }

        std::vector<SparseEntry> m_sparse;
        std::vector<Entity> m_entities;
        std::vector<T> m_components;
    };

} // namespace nbx