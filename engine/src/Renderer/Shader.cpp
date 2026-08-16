#include "Nebrix/Renderer/Shader.h"

#include "Nebrix/Core/Log.h"

#include <glad/glad.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace nbx
{

    Shader::~Shader()
    {
        if (m_id)
            glDeleteProgram(m_id);
    }

    Shader::Shader(Shader &&other) noexcept
        : m_id(other.m_id),
          m_uniformCache(std::move(other.m_uniformCache)),
          m_vertexPath(std::move(other.m_vertexPath)),
          m_fragmentPath(std::move(other.m_fragmentPath)),
          m_vertexWriteTime(other.m_vertexWriteTime),
          m_fragmentWriteTime(other.m_fragmentWriteTime),
          m_pollTimer(other.m_pollTimer)
    {
        other.m_id = 0;
    }

    Shader &Shader::operator=(Shader &&other) noexcept
    {
        if (this != &other)
        {
            if (m_id)
                glDeleteProgram(m_id);
            m_id = other.m_id;
            m_uniformCache = std::move(other.m_uniformCache);
            m_vertexPath = std::move(other.m_vertexPath);
            m_fragmentPath = std::move(other.m_fragmentPath);
            m_vertexWriteTime = other.m_vertexWriteTime;
            m_fragmentWriteTime = other.m_fragmentWriteTime;
            m_pollTimer = other.m_pollTimer;
            other.m_id = 0;
        }
        return *this;
    }

    uint32_t Shader::compile(uint32_t type, const std::string &source)
    {
        const uint32_t shader = glCreateShader(type);
        const char *sourcePtr = source.c_str();
        glShaderSource(shader, 1, &sourcePtr, nullptr);
        glCompileShader(shader);

        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[1024] = {};
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            NBX_LOG_ERROR("Shader compile error ({}): {}", type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                          infoLog);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    bool Shader::loadFromSource(const std::string &vertexSrc, const std::string &fragmentSrc)
    {
        if (m_id)
            glDeleteProgram(m_id);

        const uint32_t vertex = compile(GL_VERTEX_SHADER, vertexSrc);
        const uint32_t fragment = compile(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vertex || !fragment)
        {
            if (vertex)
                glDeleteShader(vertex);
            if (fragment)
                glDeleteShader(fragment);
            return false;
        }

        m_id = glCreateProgram();
        glAttachShader(m_id, vertex);
        glAttachShader(m_id, fragment);
        glLinkProgram(m_id);

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        int success = 0;
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[1024] = {};
            glGetProgramInfoLog(m_id, sizeof(infoLog), nullptr, infoLog);
            NBX_LOG_ERROR("Shader link error: {}", infoLog);
            glDeleteProgram(m_id);
            m_id = 0;
            return false;
        }

        m_uniformCache.clear();
        NBX_LOG_INFO("Shader compiled and linked (id={})", m_id);
        return true;
    }

    bool Shader::loadFromFile(const std::string &vertexPath, const std::string &fragmentPath)
    {
        auto readFile = [](const std::string &path, std::string &out)
        {
            std::ifstream stream(path, std::ios::binary);
            if (!stream)
            {
                NBX_LOG_ERROR("Failed to open shader file: {}", path);
                return false;
            }
            std::ostringstream buffer;
            buffer << stream.rdbuf();
            out = buffer.str();
            return true;
        };

        std::string vertexSrc;
        std::string fragmentSrc;
        if (!readFile(vertexPath, vertexSrc) || !readFile(fragmentPath, fragmentSrc))
            return false;
        if (!loadFromSource(vertexSrc, fragmentSrc))
            return false;

        m_vertexPath = vertexPath;
        m_fragmentPath = fragmentPath;
        m_vertexWriteTime = std::filesystem::last_write_time(vertexPath);
        m_fragmentWriteTime = std::filesystem::last_write_time(fragmentPath);
        NBX_LOG_INFO("Hot reload enabled for '{}' and '{}'", vertexPath, fragmentPath);
        return true;
    }

    bool Shader::update(double dt)
    {
        if (m_vertexPath.empty())
            return false;

        m_pollTimer += dt;
        if (m_pollTimer < 0.5)
            return false;
        m_pollTimer = 0.0;

        std::error_code error;
        const auto vertexWriteTime = std::filesystem::last_write_time(m_vertexPath, error);
        if (error)
        {
            NBX_LOG_WARN("Shader file vanished: {}", m_vertexPath);
            return false;
        }
        const auto fragmentWriteTime = std::filesystem::last_write_time(m_fragmentPath, error);
        if (error)
        {
            NBX_LOG_WARN("Shader file vanished: {}", m_fragmentPath);
            return false;
        }

        if (vertexWriteTime != m_vertexWriteTime || fragmentWriteTime != m_fragmentWriteTime)
        {
            NBX_LOG_INFO("Shader files changed, reloading...");
            if (loadFromFile(m_vertexPath, m_fragmentPath))
                return true;
            NBX_LOG_ERROR("Shader reload failed; keeping previous program");
        }
        return false;
    }

    void Shader::bind() const { glUseProgram(m_id); }

    void Shader::unbind() const { glUseProgram(0); }

    int32_t Shader::uniformLocation(const std::string &name)
    {
        if (const auto it = m_uniformCache.find(name); it != m_uniformCache.end())
            return it->second;

        const int32_t location = glGetUniformLocation(m_id, name.c_str());
        m_uniformCache[name] = location;
        return location;
    }

    void Shader::setInt(const std::string &name, int value)
    {
        glUniform1i(uniformLocation(name), value);
    }

    void Shader::setFloat(const std::string &name, float value)
    {
        glUniform1f(uniformLocation(name), value);
    }

    void Shader::setVec2(const std::string &name, float x, float y)
    {
        glUniform2f(uniformLocation(name), x, y);
    }

    void Shader::setVec4(const std::string &name, float x, float y, float z, float w)
    {
        glUniform4f(uniformLocation(name), x, y, z, w);
    }

    void Shader::setMat4(const std::string &name, const float *data)
    {
        glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, data);
    }

} // namespace nbx
