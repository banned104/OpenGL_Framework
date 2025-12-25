// triangle_config.hpp
// 单一职责: Triangle渲染器的专用配置
#pragma once
#include "../irender_config.hpp"
#include <vector>
#include <glm/glm.hpp>

// 包含着色器源码
#ifdef __ANDROID__
    #include <triangle/triangle.vert.es.h>
    #include <triangle/triangle.frag.es.h>
#else
    #include <triangle/triangle.vert.core.h>
    #include <triangle/triangle.frag.core.h>
#endif

// Triangle 专用顶点数据结构
struct TriangleVertex {
    glm::vec3 position;
    glm::vec3 color;
};

class TriangleConfig : public IRenderConfig {
public:
    TriangleConfig() {
        m_vertexShader = TRIANGLE_VERTEX_SHADER;
        m_fragmentShader = TRIANGLE_FRAGMENT_SHADER;
        m_clearColor = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f);
        m_rotationSpeed = 1.0f;

        // 默认三角形顶点
        m_vertices = {
            { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3( 0.0f,  0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3( 0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }
        };
    }

    // IRenderConfig 接口实现
    const std::string& vertexShaderSource() const override { return m_vertexShader; }
    const std::string& fragmentShaderSource() const override { return m_fragmentShader; }
    glm::vec4 clearColor() const override { return m_clearColor; }
    float rotationSpeed() const override { return m_rotationSpeed; }
    
    const void* vertexData() const override { return m_vertices.data(); }
    size_t vertexCount() const override { return m_vertices.size(); }
    size_t vertexStride() const override { return sizeof(TriangleVertex); }

    // Triangle 专用访问器
    const std::vector<TriangleVertex>& vertices() const { return m_vertices; }

    // Builder 方法
    TriangleConfig& setVertices(const std::vector<TriangleVertex>& v) { m_vertices = v; return *this; }
    TriangleConfig& setClearColor(const glm::vec4& c) { m_clearColor = c; return *this; }
    TriangleConfig& setRotationSpeed(float s) { m_rotationSpeed = s; return *this; }

private:
    std::string m_vertexShader;
    std::string m_fragmentShader;
    std::vector<TriangleVertex> m_vertices;
    glm::vec4 m_clearColor;
    float m_rotationSpeed;
};
