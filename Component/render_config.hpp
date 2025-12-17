// 单一职责: 封装渲染器的静态配置信息
#pragma once
#include <QString>
#include <QVector3D>
#include <vector>


struct VertexData
{
    QVector3D position;
    QVector3D color;
};

class RenderConfig {
public:
    RenderConfig() = default;

    // Builder 模式
    RenderConfig& setVertexShaderPath( const QString& path ) {
        m_vertexShaderPath = path;
        return *this;
    }

    RenderConfig& setFragmentShaderPath( const QString& path ) {
        m_fragmentShaderPath = path;
        return *this;
    }

    RenderConfig& setVertexData( const std::vector<VertexData>& data ) {
        m_vertexData = data;
        return *this;
    }

    RenderConfig& setClearColor( float r, float g, float b, float a ) {
        m_clearColor = QVector4D( r, g, b, a );
        return *this;
    }

    RenderConfig& setRotationSpeeed( float speed ) {
        m_rotationSpeed = speed;
        return *this;
    }

    // Getters
    QString vertexShaderPath() const { return m_vertexShaderPath; }
    QString fragmentShaderPath() const { return m_fragmentShaderPath; }
    const std::vector<VertexData>& vertexData() const { return m_vertexData; }
    QVector4D clearColor() const { return m_clearColor; }
    float rotationSpeed() const { return m_rotationSpeed; }


    /* ------------------------------------------------
     * 生成三角形渲染的config
     * 使用Builder模式, 链式调用,易于构建复杂配置
     * 配置和实现分离 易于修改和测试
    * ------------------------------------------------ */
    static RenderConfig createTriangleConfig() {
        RenderConfig config;

#ifdef Q_OS_WIN
    config.setFragmentShaderPath(":/src/Shaders/triangle.frag.glsl")
            .setVertexShaderPath(":/src/Shaders/triangle.vert.glsl");

#else
        config.setFragmentShaderPath(":/src/Shaders/triangle.es.frag.glsl")
            .setVertexShaderPath(":/src/Shaders/triangle.es.vert.glsl");
#endif

        std::vector<VertexData> vertices = {
            { QVector3D(-0.5f, -0.5f, 0.0f), QVector3D(1.0, 0.0, 0.0) },
            { QVector3D(0.0f, 0.5f, 0.0f),   QVector3D(0.0, 1.0, 0.0) },
            { QVector3D(0.5f, -0.5f, 0.0f),  QVector3D(0.0, 0.0, 1.0) }
        };

        config.setVertexData(vertices)
            .setClearColor(0.0f, 0.0f, 0.5f, 1.0f)
            .setRotationSpeeed(1.0f);

        return config;
    }


private:
    QString m_vertexShaderPath;
    QString m_fragmentShaderPath;
    std::vector<VertexData> m_vertexData;
    QVector4D m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };   // 为什么不是 () 而是 {}?
    float m_rotationSpeed{1.0f};
};
