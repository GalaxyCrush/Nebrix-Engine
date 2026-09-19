#pragma once

#include <Nebrix/ECS/Components.h>

#include <cstdint>

namespace nbx
{

    // Builds a fully-connected maze tilemap via recursive backtracker:
    // (tilesPerSide / 3) x (tilesPerSide / 3) cells of 2x2 tiles separated by
    // 1-tile walls, carved with a deterministic seed. Every carved cell is
    // reachable from cell (0,0). Floor cells use tile 0, walls use the bright
    // palette (4 + (x + y) % 12).
    Tilemap buildMaze(uint32_t tilesPerSide, float tileSize);

} // namespace nbx
