#pragma once

#include <cstdint>
#include <string>

namespace nbx {

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Loads an image (png, jpg, bmp, tga, gif...) from disk, RGBA8.
    bool loadFromFile(const std::string& path);

    // Creates a texture from raw RGBA8 pixels.
    bool create(uint32_t width, uint32_t height, const void* pixels);

    void bind(uint32_t slot = 0) const;
    void unbind() const;

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    uint32_t id() const { return m_id; }

private:
    uint32_t m_id = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace nbx
