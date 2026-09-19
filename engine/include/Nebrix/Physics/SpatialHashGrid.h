#pragma once

#include "Nebrix/ECS/Entity.h"
#include "Nebrix/Physics/AABB.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace nbx {

// Broadphase: buckets axis-aligned boxes into uniform cells so nearby
// candidates can be found without checking everything. Rebuild each step.
class SpatialHashGrid {
public:
    void setCellSize(float cellSize);

    void clear();
    void insert(const Entity& entity, const AABB& box);
    // Appends entities whose boxes may overlap the query box. Not deduplicated:
    // an entity spanning several cells can be returned more than once.
    void query(const AABB& box, std::vector<Entity>& out) const;

private:
    static uint64_t cellKey(int32_t cellX, int32_t cellY);

    float m_cellSize = 128.0f;
    std::unordered_map<uint64_t, std::vector<Entity>> m_cells;
};

} // namespace nbx