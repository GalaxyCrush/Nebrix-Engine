#include "Nebrix/Renderer/Texture.h"

#include "Nebrix/Core/Log.h"

#include <glad/glad.h>
#include <stb_image.h>

namespace nbx {

Texture::~Texture() {
    if (m_id)
        glDeleteTextures(1, &m_id);
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height) {
    other.m_id = 0;
    other.m_width = 0;
    other.m_height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_id)
            glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_id = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& path) {
    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        NBX_LOG_ERROR("Failed to load texture '{}': {}", path, stbi_failure_reason());
        return false;
    }

    const bool ok = create(static_cast<uint32_t>(width), static_cast<uint32_t>(height), data);
    stbi_image_free(data);

    if (ok)
        NBX_LOG_INFO("Texture loaded: '{}' ({}x{})", path, width, height);
    return ok;
}

bool Texture::create(uint32_t width, uint32_t height, const void* pixels) {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    glGenTextures(1, &m_id);
    bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    m_width = width;
    m_height = height;
    unbind();
    return true;
}

void Texture::bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

} // namespace nbx
