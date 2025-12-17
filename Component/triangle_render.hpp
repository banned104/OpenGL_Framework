#pragma once
#include "irenderer.hpp"
#include "render_config.hpp"
#include "render_context.hpp"

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

class TriangleRender : protected QOpenGLFunctions, public IRenderer
{
public:
    TriangleRender();
    ~TriangleRender() override;

    bool initialize(const RenderConfig& config) override;
    bool render( const RenderContext& context ) override;
    bool resize( int width, int height ) override;
    void cleanup() override;
    void setErrorCallback( ErrorCallback callback ) override;
    std::string getName() const override { return "TriangleRender"; };

private:
    bool initializeShader( const QString& vertexPath, const QString& fragmentShader );
    bool initializeGeometry( const std::vector<VertexData>& vertices );
    void reportError( RenderError error, const std::string& message );

    QOpenGLShaderProgram m_program;
    QOpenGLBuffer m_vbo;
    QMatrix4x4 m_projection;
    QVector4D m_clearColor;
    float m_rotationSpeed;
    float m_currentAngle;
    int m_vertexCount;

    ErrorCallback m_errorCallback;
    bool m_initialized;
};


