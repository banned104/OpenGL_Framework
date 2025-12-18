#pragma once
#include "irenderer.hpp"
#include "render_config.hpp"
#include "render_context.hpp"
#include "shader.hpp"

// 平台条件编译：OpenGL ES (Android) vs OpenGL Core (PC)
#ifdef __ANDROID__
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>
#else
    #include <glad/glad.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class TriangleRender : public IRenderer
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
    bool initializeGeometry( const std::vector<VertexData>& vertices );
    void reportError( RenderError error, const std::string& message );

    Shader m_shader;
    GLuint m_vao;
    GLuint m_vbo;
    glm::mat4 m_projection;
    glm::vec4 m_clearColor;
    float m_rotationSpeed;
    float m_currentAngle;
    int m_vertexCount;

    ErrorCallback m_errorCallback;
    bool m_initialized;
};


