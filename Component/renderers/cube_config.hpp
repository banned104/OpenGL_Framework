// cube_config.hpp
// 单一职责: Cube渲染器的专用配置
#pragma once
#include "../irender_config.hpp"
#include <vector>
#include <glm/glm.hpp>

// 包含着色器源码
#ifdef __ANDROID__
    #include <cube/cube.vert.es.h>
    #include <cube/cube.frag.es.h>
#else
    #include <cube/cube.vert.core.h>
    #include <cube/cube.frag.core.h>
#endif

// Cube 专用顶点数据结构
struct CubeVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

class CubeConfig : public IRenderConfig {
public:
    CubeConfig() {
        m_vertexShader = CUBE_VERTEX_SHADER;
        m_fragmentShader = CUBE_FRAGMENT_SHADER;
        m_clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        m_rotationSpeed = 1.0f;

        // 默认平面顶点 (两个三角形组成矩形)
        m_vertices = {
            // 第一个三角形
            { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
            { glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
            { glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },
            // 第二个三角形
            { glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },
            { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
            { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) }
        };
    }

    // IRenderConfig 接口实现
    const std::string& vertexShaderSource() const override { return m_vertexShader; }
    const std::string& fragmentShaderSource() const override { return m_fragmentShader; }
    glm::vec4 clearColor() const override { return m_clearColor; }
    float rotationSpeed() const override { return m_rotationSpeed; }

    const void* vertexData() const override { return m_vertices.data(); }
    size_t vertexCount() const override { return m_vertices.size(); }
    size_t vertexStride() const override { return sizeof(CubeVertex); }

    // Cube 专用访问器
    const std::vector<CubeVertex>& vertices() const { return m_vertices; }

    // Builder 方法
    CubeConfig& setVertices(const std::vector<CubeVertex>& v) { m_vertices = v; return *this; }
    CubeConfig& setClearColor(const glm::vec4& c) { m_clearColor = c; return *this; }
    CubeConfig& setRotationSpeed(float s) { m_rotationSpeed = s; return *this; }

private:
    std::string m_vertexShader;
    std::string m_fragmentShader;
    std::vector<CubeVertex> m_vertices;
    glm::vec4 m_clearColor;
    float m_rotationSpeed;
};
