#include "triangle_render.hpp"
#include <QDebug>

TriangleRender::TriangleRender()
    : m_vbo( QOpenGLBuffer::VertexBuffer )
    , m_clearColor( 0.0f, 0.0f, 0.5f, 1.0f )
    , m_rotationSpeed(1.0f)
    , m_currentAngle(0.0f)
    , m_vertexCount(0)
    , m_initialized(false)
{
}

TriangleRender::~TriangleRender() {
    this->cleanup();
}

bool TriangleRender::initialize( const RenderConfig& config ) {
    initializeOpenGLFunctions();

    // 初始化着色器
    if ( !initializeShader(config.vertexShaderPath(), config.fragmentShaderPath()) ) {
        reportError( RenderError::ShaderCompilationFailed, "Failed to compile shader" );
        return false;
    }

    // 初始化几何体
    if ( !initializeGeometry( config.vertexData() ) ) {
        reportError( RenderError::BufferCreationFailed, "Failed to create vertex buffer" );
    }

    // 保存配置
    m_clearColor = config.clearColor();
    m_rotationSpeed = config.rotationSpeed();
    m_initialized = true;

    return true;
}


bool TriangleRender::render( const RenderContext& context ) {
    if ( !m_initialized ) {
        reportError(RenderError::InitializationFailed, "Render not initialized");
        return false;
    }

    // glClearColor( m_clearColor.x(), m_clearColor.y(), m_clearColor.z(), m_clearColor.w() );
    glClearColor( 0.0, 0.0, 0.0, 0.0);
    glClear( GL_COLOR_BUFFER_BIT );

    m_currentAngle += m_rotationSpeed;
    if ( m_currentAngle > 360.0f  ) { m_currentAngle -= 360.0f; }

    // 模型矩阵
    QMatrix4x4 modelMatrix;
    modelMatrix.translate(0.0f, 0.0f, -5.0f);
    modelMatrix.rotate( m_currentAngle, 0.0f, 0.0f );

    // MVP 矩阵
    QMatrix4x4 mvp = context.projectionMatrix() * modelMatrix;

    // 绑定着色器程序
    if ( !m_program.bind() ) {
        reportError(RenderError::RenderingFailed, "Failed to bind shader program");
        return false;
    }

    // 设置uniform
    m_program.setUniformValue( "mvp", mvp );

    // 绑定vbo
    m_vbo.bind();

    int location = 0;

    // 顶点位置
    m_program.enableAttributeArray( location );
    m_program.setAttributeBuffer( location, GL_FLOAT, 0, 3, sizeof( VertexData ) );
    // 顶点颜色
    m_program.enableAttributeArray(location + 1);
    m_program.setAttributeBuffer(location + 1, GL_FLOAT, sizeof(QVector3D), 3, sizeof(VertexData) );

    glDrawArrays( GL_TRIANGLES, 0, m_vertexCount );

    m_vbo.release();
    m_program.release();

    return true;
}

bool TriangleRender::resize( int width, int height ) {
    glViewport( 0, 0, width, height);
    qreal aspect = qreal(width)/qreal(height);
    m_projection.setToIdentity();   // 单位矩阵
    m_projection.perspective( 30.0f, aspect, 3.0f, 10.0f );
    return true;
}

void TriangleRender::cleanup() {
    if ( m_vbo.isCreated() ) {
        m_vbo.destroy();
    }
    m_initialized = false;
}


void TriangleRender::setErrorCallback( ErrorCallback callback ) {
    m_errorCallback = callback;
}

bool TriangleRender::initializeShader(const QString& vertexPath, const QString& fragmentPath)
{
#if QT_VERSION_MINOR >= 9
    if (!m_program.addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, vertexPath)) {
        qDebug() << "Vertex shader error:" << m_program.log();
        return false;
    }
    if (!m_program.addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, fragmentPath)) {
        qDebug() << "Fragment shader error:" << m_program.log();
        return false;
    }
#else
    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, vertexPath)) {
        qDebug() << "Vertex shader error:" << m_program.log();
        return false;
    }
    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentPath)) {
        qDebug() << "Fragment shader error:" << m_program.log();
        return false;
    }
#endif

    if (!m_program.link()) {
        qDebug() << "Shader link error:" << m_program.log();
        return false;
    }

    if (!m_program.bind()) {
        qDebug() << "Shader bind error:" << m_program.log();
        return false;
    }

    return true;
}


bool TriangleRender::initializeGeometry( const std::vector<VertexData>& vertices ) {
    if ( vertices.empty() ) {
        return false;
    }

    m_vertexCount = static_cast<int>( vertices.size() );

    if ( !m_vbo.create() ) {
        return false;
    }

    m_vbo.bind();
    m_vbo.allocate( vertices.data(), m_vertexCount*sizeof( VertexData ) );
    m_vbo.release();

    return true;
}

void TriangleRender::reportError( RenderError error, const std::string& message ) {
    if ( m_errorCallback ) {
        m_errorCallback( error, message );
    }
}


