#include "Nebrix/Physics/SpatialHashGrid.h"

#include <cmath>

namespace nbx
{

    void SpatialHashGrid::setCellSize(float cellSize)
    {
        m_cellSize = cellSize > 0.0f ? cellSize : 128.0f;
    }

    void SpatialHashGrid::clear() { m_cells.clear(); }

    void SpatialHashGrid::insert(const Entity &entity, const AABB &box)
    {
        const int32_t minX = static_cast<int32_t>(std::floor(box.min.x / m_cellSize));
        const int32_t minY = static_cast<int32_t>(std::floor(box.min.y / m_cellSize));
        const int32_t maxX = static_cast<int32_t>(std::floor(box.max.x / m_cellSize));
        const int32_t maxY = static_cast<int32_t>(std::floor(box.max.y / m_cellSize));
        for (int32_t cy = minY; cy <= maxY; ++cy)
            for (int32_t cx = minX; cx <= maxX; ++cx)
                m_cells[cellKey(cx, cy)].push_back(entity);
    }

    void SpatialHashGrid::query(const AABB &box, std::vector<Entity> &out) const
    {
        const int32_t minX = static_cast<int32_t>(std::floor(box.min.x / m_cellSize));
        const int32_t minY = static_cast<int32_t>(std::floor(box.min.y / m_cellSize));
        const int32_t maxX = static_cast<int32_t>(std::floor(box.max.x / m_cellSize));
        const int32_t maxY = static_cast<int32_t>(std::floor(box.max.y / m_cellSize));
        for (int32_t cy = minY; cy <= maxY; ++cy)
        {
            for (int32_t cx = minX; cx <= maxX; ++cx)
            {
                const auto it = m_cells.find(cellKey(cx, cy));
                if (it == m_cells.end())
                    continue;
                out.insert(out.end(), it->second.begin(), it->second.end());
            }
        }
    }

    uint64_t SpatialHashGrid::cellKey(int32_t cellX, int32_t cellY)
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(cellX)) << 32) |
               static_cast<uint32_t>(cellY);
    }

} // namespace nbx