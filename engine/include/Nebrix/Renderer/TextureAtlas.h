#pragma once

#include "Nebrix/Renderer/Sprite.h"
#include "Nebrix/Renderer/Texture.h"

#include <cstdint>
#include <string>

namespace nbx
{

    // Splits a texture (sprite sheet) into a uniform grid of cells and hands out
    // one Sprite per cell. Owns the underlying Texture — do not move the atlas
    // after handing out sprites.
    class TextureAtlas
    {
    public:
        TextureAtlas() = default;

        // Loads an image and slices it into (columns x rows) cells.
        bool loadFromFile(const std::string &path, uint32_t columns, uint32_t rows);

        // Takes ownership of an existing texture and slices it into cells.
        bool create(Texture texture, uint32_t columns, uint32_t rows);

        Sprite cell(uint32_t column, uint32_t row) const;
        Sprite cell(uint32_t index) const;

        uint32_t columns() const { return m_columns; }
        uint32_t rows() const { return m_rows; }
        uint32_t cellCount() const { return m_columns * m_rows; }
        float cellWidth() const { return m_cellWidth; }
        float cellHeight() const { return m_cellHeight; }

        const Texture &texture() const { return m_texture; }

    private:
        Texture m_texture;
        uint32_t m_columns = 1;
        uint32_t m_rows = 1;
        float m_cellWidth = 0.0f;
        float m_cellHeight = 0.0f;
    };

} // namespace nbx