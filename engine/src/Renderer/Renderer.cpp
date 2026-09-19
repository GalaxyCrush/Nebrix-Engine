#include "Nebrix/Renderer/Renderer.h"

#include "Nebrix/Core/Log.h"
#include "Nebrix/Renderer/Buffer.h"
#include "Nebrix/Renderer/Shader.h"
#include "Nebrix/Renderer/Texture.h"

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <array>
#include <memory>

namespace nbx
{

    namespace
    {

        constexpr uint32_t kMaxQuads = 10'000;
        constexpr uint32_t kMaxVertices = kMaxQuads * 4;
        constexpr uint32_t kMaxIndices = kMaxQuads * 6;

        constexpr const char *kVertexShaderSource = R"(#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_Projection;

out vec2 v_TexCoord;
out vec4 v_Color;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    gl_Position = u_Projection * vec4(a_Position, 1.0);
}
)";

        constexpr const char *kFragmentShaderSource = R"(#version 330 core

in vec2 v_TexCoord;
in vec4 v_Color;

uniform sampler2D u_Texture;

out vec4 FragColor;

void main() {
    FragColor = texture(u_Texture, v_TexCoord) * v_Color;
}
)";

        struct BatchVertex
        {
            float x, y, z;
            float u, v;
            uint8_t r, g, b, a;
        };

        std::unique_ptr<Shader> s_defaultShader;
        Shader *s_shader = nullptr;
        std::unique_ptr<Texture> s_whiteTexture;
        std::unique_ptr<VertexArray> s_quadVA;
        std::unique_ptr<VertexBuffer> s_quadVB;
        std::unique_ptr<IndexBuffer> s_quadIB;
        std::array<BatchVertex, kMaxVertices> s_vertices{};

        std::unique_ptr<IndexBuffer> makeIndexBuffer()
        {
            std::array<uint32_t, kMaxIndices> indices{};
            for (uint32_t i = 0; i < kMaxQuads; ++i)
            {
                const uint32_t offset = i * 4;
                indices[i * 6 + 0] = offset + 0;
                indices[i * 6 + 1] = offset + 1;
                indices[i * 6 + 2] = offset + 2;
                indices[i * 6 + 3] = offset + 2;
                indices[i * 6 + 4] = offset + 3;
                indices[i * 6 + 5] = offset + 0;
            }
            return std::make_unique<IndexBuffer>(indices.data(), indices.size());
        }

#ifdef NEBRIX_DEBUG
        void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                        GLsizei length, const GLchar *message, const void *userParam)
        {
            (void)source;
            (void)type;
            (void)id;
            (void)length;
            (void)userParam;

            LogLevel level = LogLevel::Info;
            if (severity == GL_DEBUG_SEVERITY_HIGH)
                level = LogLevel::Error;
            else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
                level = LogLevel::Warn;

            Log::write(level, std::source_location::current(), "[OpenGL] {}",
                       message ? message : "");
        }
#endif

    } // namespace

    bool Renderer::s_initialized = false;
    uint32_t Renderer::s_quadCount = 0;
    uint32_t Renderer::s_currentTextureId = 0;
    math::mat4 Renderer::s_projection = math::mat4::identity();
    Renderer::Stats Renderer::s_stats;

    bool Renderer::init()
    {
        if (s_initialized)
        {
            NBX_LOG_WARN("Renderer already initialized");
            return true;
        }

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            NBX_LOG_ERROR("Failed to load OpenGL functions (glad)");
            return false;
        }

        NBX_LOG_INFO("OpenGL {}.{} renderer: {}", GLVersion.major, GLVersion.minor,
                     reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

#ifdef NEBRIX_DEBUG
        int contextFlags = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
        if (contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(glDebugCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            NBX_LOG_INFO("OpenGL debug output enabled");
        }
#endif

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        s_defaultShader = std::make_unique<Shader>();
        if (!s_defaultShader->loadFromSource(kVertexShaderSource, kFragmentShaderSource))
            return false;
        s_shader = s_defaultShader.get();

        s_whiteTexture = std::make_unique<Texture>();
        constexpr std::array<uint8_t, 4> white = {255, 255, 255, 255};
        s_whiteTexture->create(1, 1, white.data());

        // Static index buffer covers the whole capacity; the vertex buffer is
        // re-uploaded per flush.
        // Dynamic: the vertex buffer is re-uploaded (sub-data) on every flush.
        s_quadVB = std::make_unique<VertexBuffer>(nullptr, kMaxVertices * sizeof(BatchVertex),
                                                  BufferUsage::Dynamic);
        s_quadIB = makeIndexBuffer();

        s_quadVA = std::make_unique<VertexArray>();
        VertexBufferLayout layout = {
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float2, "a_TexCoord"},
            {ShaderDataType::UByte4, "a_Color", true},
        };
        s_quadVA->addVertexBuffer(*s_quadVB, layout);
        s_quadVA->setIndexBuffer(*s_quadIB);

        s_initialized = true;
        NBX_LOG_INFO("Renderer initialized (batch capacity: {} quads)", kMaxQuads);
        return true;
    }

    void Renderer::shutdown()
    {
        if (!s_initialized)
            return;
        s_quadVA.reset();
        s_quadIB.reset();
        s_quadVB.reset();
        s_whiteTexture.reset();
        s_defaultShader.reset();
        s_shader = nullptr;
        s_initialized = false;
        NBX_LOG_INFO("Renderer shut down");
    }

    void Renderer::setViewport(int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void Renderer::beginFrame(const math::mat4 &projection)
    {
        s_projection = projection;
        s_quadCount = 0;
        s_currentTextureId = 0;
        s_stats = {};
    }

    void Renderer::endFrame()
    {
        flush();
    }

    void Renderer::clear(const math::vec4 &color)
    {
        glClearColor(color.x, color.y, color.z, color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Renderer::drawQuad(const math::Transform2D &transform, const Sprite &sprite,
                            const math::vec4 &tint)
    {
        if (sprite.textureId != s_currentTextureId)
            flush();
        s_currentTextureId = sprite.textureId;
        submit(transform, sprite.uv, tint);
    }

    void Renderer::drawQuad(const math::Transform2D &transform, const math::vec4 &color)
    {
        if (s_whiteTexture->id() != s_currentTextureId)
            flush();
        s_currentTextureId = s_whiteTexture->id();
        submit(transform, {0.0f, 0.0f, 1.0f, 1.0f}, color);
    }

    void Renderer::setShader(Shader *shader)
    {
        Shader *next = shader ? shader : s_defaultShader.get();
        // Pending quads were expanded for the current shader; flush before switching.
        if (s_shader != next && s_quadCount > 0)
            flush();
        s_shader = next;
    }

    const Renderer::Stats &Renderer::stats() { return s_stats; }

    void Renderer::flush()
    {
        if (s_quadCount == 0)
        {
            s_currentTextureId = 0;
            return;
        }

        s_quadVB->setData(s_vertices.data(), s_quadCount * 4 * sizeof(BatchVertex));

        s_shader->bind();
        s_shader->setMat4("u_Projection", s_projection.data());
        s_shader->setInt("u_Texture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_currentTextureId);

        s_quadVA->bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_quadCount * 6), GL_UNSIGNED_INT, nullptr);
        s_quadVA->unbind();

        s_stats.drawCalls += 1;
        s_quadCount = 0;
        s_currentTextureId = 0;
    }

    void Renderer::submit(const math::Transform2D &transform, const math::vec4 &uv,
                          const math::vec4 &tint)
    {
        if (s_quadCount >= kMaxQuads)
        {
            // Flush-and-retry instead of dropping: >capacity in one frame is a
            // slowdown, not silent visual loss.
            NBX_LOG_WARN("Batch capacity reached ({}, flushing mid-frame)", kMaxQuads);
            flush();
        }

        const std::array<math::vec2, 4> kLocalCorners = {
            math::vec2{-0.5f, -0.5f},
            math::vec2{0.5f, -0.5f},
            math::vec2{0.5f, 0.5f},
            math::vec2{-0.5f, 0.5f},
        };
        const std::array<math::vec2, 4> kUvs = {
            math::vec2{uv.x, uv.y},
            math::vec2{uv.z, uv.y},
            math::vec2{uv.z, uv.w},
            math::vec2{uv.x, uv.w},
        };

        const uint32_t base = s_quadCount * 4;
        for (uint32_t i = 0; i < 4; ++i)
        {
            const math::vec2 corner = math::transformPoint(transform, kLocalCorners[i]);
            BatchVertex &vertex = s_vertices[base + i];
            vertex.x = corner.x;
            vertex.y = corner.y;
            vertex.z = 0.0f;
            vertex.u = kUvs[i].x;
            vertex.v = kUvs[i].y;
            vertex.r = static_cast<uint8_t>(tint.x * 255.0f);
            vertex.g = static_cast<uint8_t>(tint.y * 255.0f);
            vertex.b = static_cast<uint8_t>(tint.z * 255.0f);
            vertex.a = static_cast<uint8_t>(tint.w * 255.0f);
        }

        s_quadCount += 1;
        s_stats.quadCount += 1;
    }

} // namespace nbx