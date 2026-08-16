#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace nbx {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);
    // Loads from files; enables hot reload via update().
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);

    // Hot reload: polls the files' modification times every ~0.5 s and reloads
    // if changed. Call once per frame; returns true when a reload happened.
    bool update(double dt);

    void bind() const;
    void unbind() const;

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, float x, float y);
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setMat4(const std::string& name, const float* data);

    uint32_t id() const { return m_id; }

private:
    static uint32_t compile(uint32_t type, const std::string& source);
    int32_t uniformLocation(const std::string& name);

    uint32_t m_id = 0;
    std::unordered_map<std::string, int32_t> m_uniformCache;

    std::string m_vertexPath;
    std::string m_fragmentPath;
    std::filesystem::file_time_type m_vertexWriteTime;
    std::filesystem::file_time_type m_fragmentWriteTime;
    double m_pollTimer = 0.0;
};

} // namespace nbx
