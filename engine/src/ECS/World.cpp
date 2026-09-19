#include "Nebrix/ECS/World.h"

namespace nbx {

World::~World() = default;

Entity World::createEntity() {
    if (!m_freeList.empty()) {
        const uint32_t index = m_freeList.back();
        m_freeList.pop_back();
        m_alive[index] = true;
        return {index, m_versions[index]};
    }
    const uint32_t index = static_cast<uint32_t>(m_versions.size());
    m_versions.push_back(0);
    m_alive.push_back(true);
    return {index, 0};
}

void World::destroyEntity(Entity entity) {
    if (!isAlive(entity))
        return;
    m_alive[entity.index] = false;
    ++m_versions[entity.index];
    for (auto& entry : m_pools)
        entry.second->onEntityDestroyed(entity.index);
    m_freeList.push_back(entity.index);
}

bool World::isAlive(Entity entity) const {
    if (!entity.valid() || entity.index >= m_versions.size())
        return false;
    return m_alive[entity.index] && m_versions[entity.index] == entity.generation;
}

} // namespace nbx