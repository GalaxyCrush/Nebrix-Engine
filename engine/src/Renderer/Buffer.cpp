#include "Nebrix/Renderer/Buffer.h"

#include <glad/glad.h>

namespace nbx
{

    namespace
    {

        GLenum dataType(ShaderDataType type)
        {
            switch (type)
            {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
                return GL_FLOAT;
            case ShaderDataType::UInt:
                return GL_UNSIGNED_INT;
            case ShaderDataType::UByte4:
                return GL_UNSIGNED_BYTE;
            }
            return GL_FLOAT;
        }

        uint32_t dataCount(ShaderDataType type)
        {
            switch (type)
            {
            case ShaderDataType::Float:
                return 1;
            case ShaderDataType::Float2:
                return 2;
            case ShaderDataType::Float3:
                return 3;
            case ShaderDataType::Float4:
                return 4;
            case ShaderDataType::UInt:
                return 1;
            case ShaderDataType::UByte4:
                return 4;
            }
            return 1;
        }

    } // namespace

    VertexBuffer::VertexBuffer(const void *data, uint32_t size)
    {
        glGenBuffers(1, &m_id);
        bind();
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    VertexBuffer::~VertexBuffer()
    {
        if (m_id)
            glDeleteBuffers(1, &m_id);
    }

    VertexBuffer::VertexBuffer(VertexBuffer &&other) noexcept : m_id(other.m_id) { other.m_id = 0; }

    VertexBuffer &VertexBuffer::operator=(VertexBuffer &&other) noexcept
    {
        if (this != &other)
        {
            if (m_id)
                glDeleteBuffers(1, &m_id);
            m_id = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    void VertexBuffer::setData(const void *data, uint32_t size) const
    {
        bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }

    void VertexBuffer::bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_id); }

    void VertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

    IndexBuffer::IndexBuffer(const uint32_t *data, uint32_t count) : m_count(count)
    {
        glGenBuffers(1, &m_id);
        bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), data, GL_STATIC_DRAW);
    }

    IndexBuffer::~IndexBuffer()
    {
        if (m_id)
            glDeleteBuffers(1, &m_id);
    }

    IndexBuffer::IndexBuffer(IndexBuffer &&other) noexcept : m_id(other.m_id), m_count(other.m_count)
    {
        other.m_id = 0;
        other.m_count = 0;
    }

    IndexBuffer &IndexBuffer::operator=(IndexBuffer &&other) noexcept
    {
        if (this != &other)
        {
            if (m_id)
                glDeleteBuffers(1, &m_id);
            m_id = other.m_id;
            m_count = other.m_count;
            other.m_id = 0;
            other.m_count = 0;
        }
        return *this;
    }

    void IndexBuffer::bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id); }

    void IndexBuffer::unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

    VertexArray::VertexArray() { glGenVertexArrays(1, &m_id); }

    VertexArray::~VertexArray()
    {
        if (m_id)
            glDeleteVertexArrays(1, &m_id);
    }

    void VertexArray::addVertexBuffer(const VertexBuffer &buffer, const VertexBufferLayout &layout)
    {
        bind();
        buffer.bind();

        uint32_t attributeIndex = 0;
        for (const auto &element : layout.elements())
        {
            glEnableVertexAttribArray(attributeIndex);
            glVertexAttribPointer(attributeIndex, static_cast<GLint>(dataCount(element.type)),
                                  dataType(element.type), element.normalized ? GL_TRUE : GL_FALSE,
                                  static_cast<GLsizei>(layout.stride()),
                                  reinterpret_cast<const void *>(element.offset));
            ++attributeIndex;
        }

        buffer.unbind();
        unbind();
    }

    void VertexArray::setIndexBuffer(const IndexBuffer &indexBuffer)
    {
        bind();
        indexBuffer.bind();
        m_indexCount = indexBuffer.count();
        unbind();
    }

    void VertexArray::bind() const { glBindVertexArray(m_id); }

    void VertexArray::unbind() const { glBindVertexArray(0); }

} // namespace nbx
