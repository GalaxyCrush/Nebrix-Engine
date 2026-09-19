#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace nbx
{

    // A tile grid stored as plain data (no per-tile entities). The owning entity's
    // Transform position is the map origin (top-left corner of cell 0,0).
    struct Tilemap
    {
        uint32_t width = 0;
        uint32_t height = 0;
        float tileSize = 64.0f;
        std::vector<uint16_t> tiles; // width * height: index into the app's tileset
        std::vector<uint8_t> solid;  // 0/1 per cell

        uint32_t indexAt(uint32_t x, uint32_t y) const { return tiles[y * width + x]; }
        bool isSolid(uint32_t x, uint32_t y) const { return solid[y * width + x] != 0; }

        // Local coordinate (world minus origin) -> cell coordinate.
        int32_t cellX(float localX) const
        {
            return static_cast<int32_t>(std::floor(localX / tileSize));
        }
        int32_t cellY(float localY) const
        {
            return static_cast<int32_t>(std::floor(localY / tileSize));
        }

        // Map-local rect -> clamped cell range. Returns false when the rect does
        // not touch the map at all (outputs are meaningless then). Single
        // implementation behind physics sweeps and render culling, so both
        // agree on which cells a rect covers.
        bool cellRange(float localMinX, float localMinY, float localMaxX, float localMaxY,
                       int32_t &outMinX, int32_t &outMinY, int32_t &outMaxX,
                       int32_t &outMaxY) const
        {
            outMinX = std::max(0, cellX(localMinX));
            outMinY = std::max(0, cellY(localMinY));
            outMaxX = std::min(static_cast<int32_t>(width) - 1, cellX(localMaxX));
            outMaxY = std::min(static_cast<int32_t>(height) - 1, cellY(localMaxY));
            return outMinX <= outMaxX && outMinY <= outMaxY;
        }
    };

} // namespace nbx
