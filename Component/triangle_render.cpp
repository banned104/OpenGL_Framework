#include "triangle_render.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

TriangleRender::TriangleRender()
    : m_shaderProgram(0)
    , m_vao(0)
    , m_vbo(0)
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
    // 初始化着色器
    if ( !initializeShader(config.vertexShaderPath(), config.fragmentShaderPath()) ) {
        reportError( RenderError::ShaderCompilationFailed, "Failed to compile shader" );
        return false;
    }

    // 初始化几何体
    if ( !initializeGeometry( config.vertexData() ) ) {
        reportError( RenderError::BufferCreationFailed, "Failed to create vertex buffer" );
        return false;
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

    glClearColor( m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    m_currentAngle += m_rotationSpeed;
    if ( m_currentAngle > 360.0f  ) { m_currentAngle -= 360.0f; }

    // 模型矩阵
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -5.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_currentAngle), glm::vec3(0.0f, 0.0f, 1.0f));

    // MVP 矩阵
    glm::mat4 mvp = context.projectionMatrix() * modelMatrix;

    // 使用着色器程序
    glUseProgram(m_shaderProgram);

    // 设置uniform
    GLint mvpLoc = glGetUniformLocation(m_shaderProgram, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    // 绑定VAO并绘制
    glBindVertexArray(m_vao);
    glDrawArrays( GL_TRIANGLES, 0, m_vertexCount );
    glBindVertexArray(0);

    return true;
}

bool TriangleRender::resize( int width, int height ) {
    glViewport( 0, 0, width, height);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    m_projection = glm::perspective( glm::radians(30.0f), aspect, 3.0f, 10.0f );
    return true;
}

void TriangleRender::cleanup() {
    if ( m_vao != 0 ) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if ( m_vbo != 0 ) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if ( m_shaderProgram != 0 ) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    m_initialized = false;
}


void TriangleRender::setErrorCallback( ErrorCallback callback ) {
    m_errorCallback = callback;
}

bool TriangleRender::initializeShader(const std::string& vertexPath, const std::string& fragmentPath)
{
    // 加载着色器源码
    std::string vertexSource = loadShaderSource(vertexPath);
    std::string fragmentSource = loadShaderSource(fragmentPath);
    
    if (vertexSource.empty() || fragmentSource.empty()) {
        std::cerr << "Failed to load shader files" << std::endl;
        return false;
    }

    // 编译着色器
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return false;
    }

    // 创建着色器程序
    m_shaderProgram = createShaderProgram(vertexShader, fragmentShader);
    
    // 删除着色器对象（已链接到程序中）
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return m_shaderProgram != 0;
}


bool TriangleRender::initializeGeometry( const std::vector<VertexData>& vertices ) {
    if ( vertices.empty() ) {
        return false;
    }

    m_vertexCount = static_cast<int>( vertices.size() );

    // 创建VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // 创建VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(VertexData), vertices.data(), GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性 (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
    
    // 颜色属性 (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, color));

    glBindVertexArray(0);

    return true;
}

std::string TriangleRender::loadShaderSource( const std::string& filepath ) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint TriangleRender::compileShader( GLenum type, const std::string& source ) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // 检查编译错误
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

GLuint TriangleRender::createShaderProgram( GLuint vertexShader, GLuint fragmentShader ) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // 检查链接错误
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking error: " << infoLog << std::endl;
        return 0;
    }

    return program;
}

void TriangleRender::reportError( RenderError error, const std::string& message ) {
    if ( m_errorCallback ) {
        m_errorCallback( error, message );
    }
}


