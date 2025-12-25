#ifdef USE_CUBE_RENDER

#include "cube_render.hpp"
#include <iostream>

CubeRender::CubeRender()
    : m_vao(0)
    , m_vbo(0)
    , m_projection(1.0f)
    , m_clearColor(0.1f, 0.1f, 0.1f, 1.0f)
    , m_rotationSpeed(1.0f)
    , m_currentAngle(0.0f)
    , m_vertexCount(0)
    , m_initialized(false)
{ }

CubeRender::~CubeRender() {
    this->cleanup();
}

bool CubeRender::initialize(const IRenderConfig& config) {
    // 向下转型获取具体配置
    const auto* cubeConfig = dynamic_cast<const CubeConfig*>(&config);
    if (!cubeConfig) {
        reportError(RenderError::InitializationFailed, "Invalid config type for CubeRender");
        return false;
    }

    if (!m_shader.loadFromSource(config.vertexShaderSource(), config.fragmentShaderSource())) {
        this->reportError(RenderError::ShaderCompilationFailed, "Failed to compile shader:" + m_shader.lastError());
        return false;
    }

    // 初始化几何体
    if (!initializeGeometry(cubeConfig->vertices())) {
        reportError(RenderError::BufferCreationFailed, "Failed to create vertex buffer");
        return false;
    }

    // 保存配置
    m_clearColor = config.clearColor();
    m_rotationSpeed = config.rotationSpeed();
    m_initialized = true;

    return true;
}

bool CubeRender::initializeGeometry(const std::vector<CubeVertex>& vertices) {
    if (vertices.empty()) {
        return false;
    }

    this->m_vertexCount = static_cast<int>(vertices.size());

    // VAO 
    glGenVertexArrays(1, &this->m_vao);
    glBindVertexArray(this->m_vao);

    // VBO
    glGenBuffers(1, &this->m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo);
    glBufferData(GL_ARRAY_BUFFER, this->m_vertexCount * sizeof(CubeVertex), vertices.data(), GL_STATIC_DRAW);

    // 位置属性 (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)0);

    // 纹理坐标属性 (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (void*)offsetof(CubeVertex, texCoord));

    glBindVertexArray(0);
    return true;
}


void CubeRender::cleanup() {
    if (this->m_vao != 0) {
        glDeleteBuffers(1, &this->m_vao);
        this->m_vao = 0;
    }

    if (this->m_vbo != 0) {
        glDeleteBuffers(1, &this->m_vbo);
        this->m_vbo = 0;
    }

    this->m_shader.release();
    this->m_initialized = false;
}


void CubeRender::setErrorCallback(ErrorCallback callback) {
    this->m_errorCallback = callback;
}

bool CubeRender::resize(int width, int height) {
    glViewport(0, 0, width, height);
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    this->m_projection = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
    return true;
}

void CubeRender::reportError(RenderError error, const std::string& msg) {
    std::cerr << "CubeRender Error: " << msg << std::endl;
    if (m_errorCallback) {
        m_errorCallback(error, msg);
    }
}

std::string CubeRender::getName() const {
    return "cube";
}



bool CubeRender::render(const RenderContext& context) {
    if (!m_initialized) {
        reportError(RenderError::InitializationFailed, "CubeRender not initialized");
        return false;
    }

    glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 更新旋转角度
    m_currentAngle += m_rotationSpeed;
    if (m_currentAngle > 360.0f) {
        m_currentAngle -= 360.0f;
    }

    // 构建模型矩阵
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -5.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_currentAngle), glm::vec3(0.0f, 0.0f, 1.0f));

    // MVP矩阵
    glm::mat4 mvp = context.projectionMatrix() * modelMatrix;

    m_shader.use();
    m_shader.setMat4("mvp", mvp);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    glBindVertexArray(0);

    m_shader.unuse();

    return true;
}


#endif