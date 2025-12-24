// 单一职责: 封装渲染器的静态配置信息
#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

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
// 前向声明
class TriangleRender;
class CubeRender;
/*------------- Traits -------------*/

template<typename RenderType>
struct RenderTraits;

// 为每个渲染器特化
template<>
struct RenderTraits<TriangleRender>
{
    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    static constexpr const char* vertexShaderSource = TRIANGLE_VERTEX_SHADER;
    static constexpr const char* fragmentShaderSource = TRIANGLE_FRAGMENT_SHADER;
};

template<>
struct RenderTraits<CubeRender> {
    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 texcoord;
    };

    static constexpr const char* vertexShaderSource = CUBE_VERTEX_SHADER;
    static constexpr const char* fragmentShaderSource = CUBE_FRAGMENT_SHADER;
};

#define USE_TRIANGLE_RENDER

#ifdef USE_TRIANGLE_RENDER
    using ActiveRenderer = TriangleRender;
#elif USE_CUBE_RENDER
    using ActiveRenderer = CubeRender;
#endif

using VertexData = RenderTraits<ActiveRenderer>::VertexData;

/*------------- Traits -------------*/



struct VertexData2DPlane {
    glm::vec3 position;
    glm::vec2 texcoord;
};

class RenderConfig {
public:
    RenderConfig() = default;

    // Builder 模式 - 设置着色器源码
    RenderConfig& setVertexShaderSource( const std::string& source ) {
        m_vertexShaderSource = source;
        return *this;
    }

    RenderConfig& setFragmentShaderSource( const std::string& source ) {
        m_fragmentShaderSource = source;
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
    const std::string& vertexShaderSource() const { return m_vertexShaderSource; }
    const std::string& fragmentShaderSource() const { return m_fragmentShaderSource; }
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

        // 使用编译时嵌入的着色器源码（从.h头文件）
        config.setVertexShaderSource(TRIANGLE_VERTEX_SHADER)
              .setFragmentShaderSource(TRIANGLE_FRAGMENT_SHADER);

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

    static RenderConfig createCubeConfig() {
        RenderConfig config;

        config.setVertexShaderSource(TRIANGLE_VERTEX_SHADER)
              .setFragmentShaderSource(TRIANGLE_FRAGMENT_SHADER);

        //std::vector<VertexData> vertices = {
        //    // 空间坐标vec3, 纹理坐标vec2
        //    { glm::vec3(-1.0, -1.0, 0.0), glm::vec2(0.0, 0.0) },
        //    { glm::vec3(-1.0,  1.0, 0.0), glm::vec2(1.0, 0.0) },
        //    { glm::vec3( 1.0,  1.0, 0.0), glm::vec2(1.0, 1.0) },
        //    { glm::vec3( 1.0, -1.0, 0.0), glm::vec2(0.0, 1.0) }
        //};

        //config.setVertexData(vertices);

        return config;
    }


private:
    std::string m_vertexShaderSource;
    std::string m_fragmentShaderSource;
    std::vector<VertexData> m_vertexData;
    glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    float m_rotationSpeed{1.0f};
};
