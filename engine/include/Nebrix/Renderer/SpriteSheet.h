#pragma once

#include "Nebrix/Core/Assert.h"
#include "Nebrix/Renderer/Sprite.h"

#include <cstdint>

namespace nbx
{

    // A uniform grid view over an existing texture (a sprite sheet). Non-owning:
    // it references the texture by id, so the texture must outlive the sheet and
    // every sprite handed out. Pairs naturally with AssetManager-owned textures.
    //
    // Indexing is row-major from the TOP-LEFT of the image, matching both how
    // image files are laid out and the engine's uv.y-is-top convention.
    class SpriteSheet
    {
    public:
        SpriteSheet() = default;

        // cellWidth/cellHeight: the region's natural pixel size (used for
        // sizing sprites in world units; 1 world unit = 1 px at zoom 1).
        SpriteSheet(uint32_t textureId, uint32_t columns, uint32_t rows, float cellWidth,
                    float cellHeight)
            : m_textureId(textureId), m_columns(columns), m_rows(rows),
              m_cellWidth(cellWidth), m_cellHeight(cellHeight)
        {
            NBX_ASSERT(m_columns > 0 && m_rows > 0, "sprite sheet grid must be non-empty");
        }

        // Sprite for a cell; index is row-major (index = row * columns + column).
        Sprite sprite(uint32_t column, uint32_t row) const
        {
            NBX_ASSERT(column < m_columns && row < m_rows, "sprite cell out of range");
            const float u0 = static_cast<float>(column) / static_cast<float>(m_columns);
            const float v0 = static_cast<float>(row) / static_cast<float>(m_rows);
            const float u1 = static_cast<float>(column + 1) / static_cast<float>(m_columns);
            const float v1 = static_cast<float>(row + 1) / static_cast<float>(m_rows);
            return {m_textureId, {u0, v0, u1, v1}, m_cellWidth, m_cellHeight};
        }

        Sprite sprite(uint32_t index) const
        {
            return sprite(index % m_columns, index / m_columns);
        }

        uint32_t columns() const { return m_columns; }
        uint32_t rows() const { return m_rows; }
        uint32_t cellCount() const { return m_columns * m_rows; }
        float cellWidth() const { return m_cellWidth; }
        float cellHeight() const { return m_cellHeight; }

    private:
        uint32_t m_textureId = 0;
        uint32_t m_columns = 1;
        uint32_t m_rows = 1;
        float m_cellWidth = 0.0f;
        float m_cellHeight = 0.0f;
    };

} // namespace nbx
