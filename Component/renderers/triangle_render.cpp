#ifdef USE_TRIANGLE_RENDER

#include "triangle_render.hpp"
#include <iostream>

TriangleRender::TriangleRender()
    : m_vao(0)
    , m_vbo(0)
    , m_projection(1.0f)
    , m_clearColor(0.0f, 0.0f, 0.5f, 1.0f)
    , m_rotationSpeed(1.0f)
    , m_currentAngle(0.0f)
    , m_vertexCount(0)
    , m_initialized(false)
{
}

TriangleRender::~TriangleRender() {
    cleanup();
}

bool TriangleRender::initialize(const IRenderConfig& config) {
    // 向下转型获取具体配置
    const auto* triangleConfig = dynamic_cast<const TriangleConfig*>(&config);
    if (!triangleConfig) {
        reportError(RenderError::InitializationFailed, "Invalid config type for TriangleRender");
        return false;
    }

    // 使用Shader类从源码加载着色器
    if (!m_shader.loadFromSource(config.vertexShaderSource(), config.fragmentShaderSource())) {
        reportError(RenderError::ShaderCompilationFailed,  "Failed to compile shader: " + m_shader.lastError());
        return false;
    }

    // 初始化几何体
    if (!initializeGeometry(triangleConfig->vertices())) {
        reportError(RenderError::BufferCreationFailed, "Failed to create vertex buffer");
        return false;
    }

    // 保存配置
    m_clearColor = config.clearColor();
    m_rotationSpeed = config.rotationSpeed();
    m_initialized = true;

    return true;
}

bool TriangleRender::render(const RenderContext& context) {
    if (!m_initialized) {
        reportError(RenderError::InitializationFailed, "Renderer not initialized");
        return false;
    }

    // 清屏
    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 更新旋转角度
    m_currentAngle += m_rotationSpeed;
    if (m_currentAngle > 360.0f) {
        m_currentAngle -= 360.0f;
    }

    // 模型矩阵
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -5.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_currentAngle), glm::vec3(0.0f, 0.0f, 1.0f));

    // MVP矩阵
    glm::mat4 mvp = context.projectionMatrix() * modelMatrix;

    // 使用着色器并设置uniform
    m_shader.use();
    m_shader.setMat4("mvp", mvp);

    // 绑定VAO并绘制
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    glBindVertexArray(0);

    m_shader.unuse();

    return true;
}

bool TriangleRender::resize(int width, int height) {
    glViewport(0, 0, width, height);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    m_projection = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
    return true;
}

void TriangleRender::cleanup() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    m_shader.release();
    m_initialized = false;
}

void TriangleRender::setErrorCallback(ErrorCallback callback) {
    m_errorCallback = callback;
}

bool TriangleRender::initializeGeometry(const std::vector<TriangleVertex>& vertices) {
    if (vertices.empty()) {
        return false;
    }

    m_vertexCount = static_cast<int>(vertices.size());

    // 创建VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // 创建VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(TriangleVertex), vertices.data(), GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性 (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)0);

    // 颜色属性 (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TriangleVertex), (void*)offsetof(TriangleVertex, color));

    glBindVertexArray(0);

    return true;
}

void TriangleRender::reportError(RenderError error, const std::string& message) {
    std::cerr << "TriangleRender Error: " << message << std::endl;
    if (m_errorCallback) {
        m_errorCallback(error, message);
    }
}


#endif