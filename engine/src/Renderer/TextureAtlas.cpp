#include "Nebrix/Renderer/TextureAtlas.h"

#include "Nebrix/Core/Log.h"

namespace nbx
{

    bool TextureAtlas::loadFromFile(const std::string &path, uint32_t columns, uint32_t rows)
    {
        Texture texture;
        if (!texture.loadFromFile(path))
            return false;
        return create(std::move(texture), columns, rows);
    }

    bool TextureAtlas::create(Texture texture, uint32_t columns, uint32_t rows)
    {
        if (columns == 0 || rows == 0)
        {
            NBX_LOG_ERROR("TextureAtlas: columns and rows must be > 0 (got {}x{})", columns, rows);
            return false;
        }
        m_texture = std::move(texture);
        m_columns = columns;
        m_rows = rows;
        m_cellWidth = static_cast<float>(m_texture.width()) / static_cast<float>(columns);
        m_cellHeight = static_cast<float>(m_texture.height()) / static_cast<float>(rows);
        NBX_LOG_INFO("TextureAtlas created: {}x{} cells of {:.0f}x{:.0f}px", columns, rows,
                     m_cellWidth, m_cellHeight);
        return true;
    }

    Sprite TextureAtlas::cell(uint32_t column, uint32_t row) const
    {
        const float u0 = static_cast<float>(column) / static_cast<float>(m_columns);
        const float v0 = static_cast<float>(row) / static_cast<float>(m_rows);
        const float u1 = static_cast<float>(column + 1) / static_cast<float>(m_columns);
        const float v1 = static_cast<float>(row + 1) / static_cast<float>(m_rows);
        return {m_texture.id(), {u0, v0, u1, v1}, m_cellWidth, m_cellHeight};
    }

    Sprite TextureAtlas::cell(uint32_t index) const
    {
        return cell(index % m_columns, index / m_columns);
    }

} // namespace nbx