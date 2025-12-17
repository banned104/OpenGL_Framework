// 单一职责: 封装渲染器的静态配置信息
#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>


struct VertexData
{
    glm::vec3 position;
    glm::vec3 color;
};

class RenderConfig {
public:
    RenderConfig() = default;

    // Builder 模式
    RenderConfig& setVertexShaderPath( const std::string& path ) {
        m_vertexShaderPath = path;
        return *this;
    }

    RenderConfig& setFragmentShaderPath( const std::string& path ) {
        m_fragmentShaderPath = path;
        return *this;
    }

    RenderConfig& setVertexData( const std::vector<VertexData>& data ) {
        m_vertexData = data;
        return *this;
    }

    RenderConfig& setClearColor( float r, float g, float b, float a ) {
        m_clearColor = glm::vec4( r, g, b, a );
        return *this;
    }

    RenderConfig& setRotationSpeeed( float speed ) {
        m_rotationSpeed = speed;
        return *this;
    }

    // Getters
    std::string vertexShaderPath() const { return m_vertexShaderPath; }
    std::string fragmentShaderPath() const { return m_fragmentShaderPath; }
    const std::vector<VertexData>& vertexData() const { return m_vertexData; }
    glm::vec4 clearColor() const { return m_clearColor; }
    float rotationSpeed() const { return m_rotationSpeed; }


    /* ------------------------------------------------
     * 生成三角形渲染的config
     * 使用Builder模式, 链式调用,易于构建复杂配置
     * 配置和实现分离 易于修改和测试
    * ------------------------------------------------ */
    static RenderConfig createTriangleConfig() {
        RenderConfig config;

        // 使用相对路径或绝对路径指向shader文件
        config.setFragmentShaderPath("shaders/triangle.frag")
              .setVertexShaderPath("shaders/triangle.vert");

        std::vector<VertexData> vertices = {
            { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3(0.0f, 0.5f, 0.0f),   glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.5f, -0.5f, 0.0f),  glm::vec3(0.0f, 0.0f, 1.0f) }
        };

        config.setVertexData(vertices)
            .setClearColor(0.0f, 0.0f, 0.5f, 1.0f)
            .setRotationSpeeed(1.0f);

        return config;
    }


private:
    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
    std::vector<VertexData> m_vertexData;
    glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    float m_rotationSpeed{1.0f};
};
