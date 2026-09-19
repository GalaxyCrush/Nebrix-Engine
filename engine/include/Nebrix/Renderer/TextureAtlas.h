#pragma once

#include "Nebrix/Renderer/Sprite.h"
#include "Nebrix/Renderer/SpriteSheet.h"
#include "Nebrix/Renderer/Texture.h"

#include <cstdint>
#include <string>

namespace nbx
{

    // Splits a texture (sprite sheet) into a uniform grid of cells and hands out
    // one Sprite per cell. Owns the underlying Texture — do not move the atlas
    // after handing out sprites. Cell math is delegated to SpriteSheet, the
    // single UV implementation shared with non-owning sheets.
    class TextureAtlas
    {
    public:
        TextureAtlas() = default;

        // Loads an image and slices it into (columns x rows) cells.
        bool loadFromFile(const std::string &path, uint32_t columns, uint32_t rows);

        // Takes ownership of an existing texture and slices it into cells.
        bool create(Texture texture, uint32_t columns, uint32_t rows);

        Sprite cell(uint32_t column, uint32_t row) const { return m_sheet.sprite(column, row); }
        Sprite cell(uint32_t index) const { return m_sheet.sprite(index); }

        uint32_t columns() const { return m_sheet.columns(); }
        uint32_t rows() const { return m_sheet.rows(); }
        uint32_t cellCount() const { return m_sheet.cellCount(); }
        float cellWidth() const { return m_sheet.cellWidth(); }
        float cellHeight() const { return m_sheet.cellHeight(); }

        const Texture &texture() const { return m_texture; }

    private:
        Texture m_texture;
        SpriteSheet m_sheet;
    };

} // namespace nbx
