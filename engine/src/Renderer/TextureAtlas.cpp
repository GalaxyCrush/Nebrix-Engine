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
        const float cellWidth =
            static_cast<float>(m_texture.width()) / static_cast<float>(columns);
        const float cellHeight =
            static_cast<float>(m_texture.height()) / static_cast<float>(rows);
        m_sheet = SpriteSheet(m_texture.id(), columns, rows, cellWidth, cellHeight);
        NBX_LOG_INFO("TextureAtlas created: {}x{} cells of {:.0f}x{:.0f}px", columns, rows,
                     cellWidth, cellHeight);
        return true;
    }

} // namespace nbx