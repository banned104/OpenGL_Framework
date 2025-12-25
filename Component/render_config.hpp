// 单一职责: 封装渲染器的静态配置信息
#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <type_traits>

// 包含生成的着色器头文件（编译时嵌入）
// 根据平台选择不同的着色器版本
// 注意：CMakeLists.txt中已将shaders目录添加到include路径
#ifdef __ANDROID__
    #include <triangle/triangle.vert.es.h>
    #include <triangle/triangle.frag.es.h>

    #include <cube/cube.vert.es.h>
    #include <cube/cube.vert.es.h>
#else
    #include <triangle/triangle.vert.core.h>
    #include <triangle/triangle.frag.core.h>

    #include <cube/cube.vert.core.h>
    #include <cube/cube.frag.core.h>
#endif

/*------------ 开闭原则 使用Traits范式优化代码 在编译期间选择Render进行渲染 ------------ */
// 定义标签类型
struct TriangleRendererTag {};
struct CubeRendererTag {};

/*------------- Traits -------------*/
template<typename RenderType>
struct RenderTraits;

// Triangle 渲染器特化
template<>
struct RenderTraits<TriangleRendererTag>
{
    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    static constexpr const char* vertexShaderSource = TRIANGLE_VERTEX_SHADER;
    static constexpr const char* fragmentShaderSource = TRIANGLE_FRAGMENT_SHADER;

    static std::vector<VertexData> getDefaultVertices() {
        return {
            { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
            { glm::vec3(0.0f, 0.5f, 0.0f),   glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.5f, -0.5f, 0.0f),  glm::vec3(0.0f, 0.0f, 1.0f) }
        };
    }

    static glm::vec4 getDefaultClearColor() {
        return glm::vec4(0.0f, 0.0f, 0.5f, 1.0f);
    }

    static float getDefaultRotationSpeed() { return 1.0f; }
};

// Cube 渲染器特化
template<>
struct RenderTraits<CubeRendererTag>
{
    struct VertexData
    {
        glm::vec3 position;
        glm::vec2 textureCoord;
    };

    static constexpr const char* vertexShaderSource = CUBE_VERTEX_SHADER;
    static constexpr const char* fragmentShaderSource = CUBE_FRAGMENT_SHADER;

    static std::vector<VertexData> getDefaultVertices() {
        return {
            { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
            { glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
            { glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },
            { glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f) }
        };
    }

    static glm::vec4 getDefaultClearColor() {
        return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    static float getDefaultRotationSpeed() { return 1.0f; }
};

/*------------- 编译期选择当前活动的渲染器 -------------*/
#ifdef USE_TRIANGLE_RENDER
    using ActiveRenderer = TriangleRendererTag;
#elif USE_CUBE_RENDER
    using ActiveRenderer = CubeRendererTag;
#else
    // 默认使用 Triangle
    using ActiveRenderer = TriangleRendererTag;
#endif

// 当前活动渲染器的顶点数据类型
using VertexData = typename RenderTraits<ActiveRenderer>::VertexData;

/*------------- RenderConfig 类 (非模板，使用当前活动渲染器类型) -------------*/
class RenderConfig {
public:
    RenderConfig() = default;

    // Builder 模式 - 设置着色器源码
    RenderConfig& setVertexShaderSource(const std::string& source) {
        m_vertexShaderSource = source;
        return *this;
    }

    RenderConfig& setFragmentShaderSource(const std::string& source) {
        m_fragmentShaderSource = source;
        return *this;
    }

    RenderConfig& setVertexData(const std::vector<VertexData>& data) {
        m_vertexData = data;
        return *this;
    }

    RenderConfig& setClearColor(float r, float g, float b, float a) {
        m_clearColor = glm::vec4(r, g, b, a);
        return *this;
    }

    RenderConfig& setRotationSpeed(float speed) {
        m_rotationSpeed = speed;
        return *this;
    }

    // Getters
    const std::string& vertexShaderSource() const { return m_vertexShaderSource; }
    const std::string& fragmentShaderSource() const { return m_fragmentShaderSource; }
    const std::vector<VertexData>& vertexData() const { return m_vertexData; }
    glm::vec4 clearColor() const { return m_clearColor; }
    float rotationSpeed() const { return m_rotationSpeed; }

    // 静态工厂方法 - 创建当前活动渲染器的默认配置
    static RenderConfig createDefaultConfig() {
        using Traits = RenderTraits<ActiveRenderer>;
        
        RenderConfig config;
        config.setVertexShaderSource(Traits::vertexShaderSource)
                .setFragmentShaderSource(Traits::fragmentShaderSource)
                .setVertexData(Traits::getDefaultVertices());
        
        config.m_clearColor = Traits::getDefaultClearColor();
        config.m_rotationSpeed = Traits::getDefaultRotationSpeed();
        
        return config;
    }

private:
    std::string m_vertexShaderSource;
    std::string m_fragmentShaderSource;
    std::vector<VertexData> m_vertexData;
    glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    float m_rotationSpeed{ 1.0f };
};


