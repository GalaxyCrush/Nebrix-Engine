#include "Maze.h"

#include <algorithm>
#include <array>
#include <random>
#include <vector>

namespace nbx
{

    Tilemap buildMaze(uint32_t tilesPerSide, float tileSize)
    {
        constexpr uint32_t kCellTiles = 2;
        const uint32_t mazeCells = tilesPerSide / 3;

        Tilemap tilemap;
        tilemap.width = tilesPerSide;
        tilemap.height = tilesPerSide;
        tilemap.tileSize = tileSize;
        tilemap.tiles.assign(tilesPerSide * tilesPerSide, 0);
        tilemap.solid.assign(tilesPerSide * tilesPerSide, 1); // start: all walls

        const auto carve = [&](uint32_t cx, uint32_t cy)
        {
            for (uint32_t y = 0; y < kCellTiles; ++y)
                for (uint32_t x = 0; x < kCellTiles; ++x)
                    tilemap.solid[(cy * 3 + y) * tilesPerSide + cx * 3 + x] = 0;
        };
        const auto cellIndex = [mazeCells](uint32_t cx, uint32_t cy)
        { return cy * mazeCells + cx; };

        std::mt19937 rng(20260817);
        std::vector<uint8_t> visited(mazeCells * mazeCells, 0);
        std::vector<std::pair<uint32_t, uint32_t>> stack;
        std::array<std::pair<int, int>, 4> directions = {{{0, 1}, {0, -1}, {1, 0}, {-1, 0}}};

        carve(0, 0);
        visited[cellIndex(0, 0)] = 1;
        stack.push_back({0, 0});
        while (!stack.empty())
        {
            const auto [cx, cy] = stack.back();
            std::shuffle(directions.begin(), directions.end(), rng);
            bool moved = false;
            for (const auto [dx, dy] : directions)
            {
                const int nx = static_cast<int>(cx) + dx;
                const int ny = static_cast<int>(cy) + dy;
                if (nx < 0 || ny < 0 || nx >= static_cast<int>(mazeCells) ||
                    ny >= static_cast<int>(mazeCells) ||
                    visited[cellIndex(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny))])
                    continue;
                // Knock down the 1-tile wall between the two cells.
                const uint32_t wallX =
                    static_cast<uint32_t>(std::min(static_cast<int>(cx), nx)) * 3 + 2;
                const uint32_t wallY =
                    static_cast<uint32_t>(std::min(static_cast<int>(cy), ny)) * 3 + 2;
                for (uint32_t k = 0; k < kCellTiles; ++k)
                {
                    if (dx != 0)
                        tilemap.solid[(cy * 3 + k) * tilesPerSide + wallX] = 0;
                    else
                        tilemap.solid[wallY * tilesPerSide + cx * 3 + k] = 0;
                }
                carve(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny));
                visited[cellIndex(static_cast<uint32_t>(nx), static_cast<uint32_t>(ny))] = 1;
                stack.push_back({static_cast<uint32_t>(nx), static_cast<uint32_t>(ny)});
                moved = true;
                break;
            }
            if (!moved)
                stack.pop_back();
        }

        // Floor cells stay index 0 (dark); walls get the bright palette.
        for (uint32_t ty = 0; ty < tilesPerSide; ++ty)
            for (uint32_t tx = 0; tx < tilesPerSide; ++tx)
                if (tilemap.solid[ty * tilesPerSide + tx])
                    tilemap.tiles[ty * tilesPerSide + tx] = 4 + (tx + ty) % 12;

        return tilemap;
    }

} // namespace nbx
