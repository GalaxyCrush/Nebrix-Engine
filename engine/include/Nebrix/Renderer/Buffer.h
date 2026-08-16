#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace nbx {

enum class ShaderDataType {
    Float,
    Float2,
    Float3,
    Float4,
    UInt,
    UByte4,  // normalized 4x8-bit color
};

struct BufferElement {
    ShaderDataType type;
    std::string name;
    uint32_t size;
    uint32_t offset;
    bool normalized;

    BufferElement(ShaderDataType type, std::string name, bool normalized = false)
        : type(type), name(std::move(name)), size(sizeOf(type)), offset(0), normalized(normalized) {}

    static uint32_t sizeOf(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float: return 4;
            case ShaderDataType::Float2: return 8;
            case ShaderDataType::Float3: return 12;
            case ShaderDataType::Float4: return 16;
            case ShaderDataType::UInt: return 4;
            case ShaderDataType::UByte4: return 4;
        }
        return 0;
    }
};

class VertexBufferLayout {
public:
    VertexBufferLayout() = default;
    VertexBufferLayout(std::initializer_list<BufferElement> elements) : m_elements(elements) {
        computeOffsetsAndStride();
    }

    const std::vector<BufferElement>& elements() const { return m_elements; }
    uint32_t stride() const { return m_stride; }

private:
    void computeOffsetsAndStride() {
        m_stride = 0;
        for (auto& element : m_elements) {
            element.offset = m_stride;
            m_stride += element.size;
        }
    }

    std::vector<BufferElement> m_elements;
    uint32_t m_stride = 0;
};

class VertexBuffer {
public:
    explicit VertexBuffer(const void* data, uint32_t size);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    void setData(const void* data, uint32_t size) const;
    void bind() const;
    void unbind() const;

    uint32_t id() const { return m_id; }

private:
    uint32_t m_id = 0;
};

class IndexBuffer {
public:
    IndexBuffer(const uint32_t* data, uint32_t count);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    void bind() const;
    void unbind() const;

    uint32_t count() const { return m_count; }
    uint32_t id() const { return m_id; }

private:
    uint32_t m_id = 0;
    uint32_t m_count = 0;
};

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void addVertexBuffer(const VertexBuffer& buffer, const VertexBufferLayout& layout);
    void setIndexBuffer(const IndexBuffer& indexBuffer);
    void bind() const;
    void unbind() const;

    uint32_t indexCount() const { return m_indexCount; }

private:
    uint32_t m_id = 0;
    uint32_t m_indexCount = 0;
};

} // namespace nbx
