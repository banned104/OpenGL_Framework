#include "cube_render.hpp"
#include <iostream>

CubeRender::CubeRender()
    : m_vao(0)
    , m_vbo(0)
    , m_projection(0)
    , m_clearColor(1.0f, 1.0f, 1.0f, 1.0f)
    , m_rotationSpeed(0)
    , m_currentAngle(0)
    , m_vertexCount(0)
    , m_initialized(false)
{ }

CubeRender::~CubeRender() {
    this->cleanup();
}

bool CubeRender::initialize(const RenderConfig& config) {
    if (!m_shader.loadFromSource(config.vertexShaderSource(), config.fragmentShaderSource())) {
        this->reportError(RenderError::ShaderCompilationFailed, "Failed to compile shader:" + m_shader.lastError());
        return false;
    }
}

bool CubeRender::initializeGeometry(const std::vector<VertexData>& vertices) {
    if (vertices.empty()) {
        return false;
    }

    this->m_vertexCount = static_cast<int>(vertices.size());

    // VAO 
    // void glGenVertexArrays(GLsizei n, GLuint *arrays); n是需要生成多少个VAO身份证号, array 用于保存多个身份证号.
    glGenVertexArrays(1, &this->m_vao);
    std::cout << "VAO Number:" << this->m_vao << std::endl;
    // 状态机变化: 激活VAO, 第一次绑定ID, 当前处理的就是这个VAO
    // glBindVertexArray binds the vertex array object with name array. 
    // array is the name of a vertex array object previously returned from a call to glGenVertexArrays, 
    // --> or zero to break the existing vertex array object binding.  <--
    glBindVertexArray(this->m_vao);

    // VBO
    glGenBuffers(1, &this->m_vbo);
    // https://registry.khronos.org/OpenGL-Refpages/gl4/ -> glBindBuffer -> GL_ARRAY_BUFFER/GL_UNIFORM_BUFFER/GL_TEXTURE_BUFFER...
    glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo);
    // std::vector<VertexData>::data 是一个常量成员函数, 返回指向向量中第一个元素的指针.  顶点属性数组;
    glBufferData(GL_ARRAY_BUFFER, this->m_vertexCount * sizeof(VertexData), vertices.data(), GL_STATIC_DRAW);

    // Enable layout(Location=0); -> Vertex Attribute Array; 顶点属性数组, 顶点属性:顶点位置,颜色,纹理坐标,法向量; 所以顶点属性数组用于保存这些东西;
    // 状态机状态变化 当前处理Location=0 的数组
    // 0 -> 顶点坐标
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(/*index*/1,/*size*/3,/*Type*/GL_FLOAT,/*Normalized*/GL_FALSE, /*Stride*/sizeof(VertexData), /*Pointer*/(void*)0);
    

    // 1 -> 顶点纹理坐标
    glEnableVertexAttribArray(1);
    // 这样写是错的; 因为 offsetof 定义是 #define offsetof(type, member)  ((size_t)&(((type*)0)->member))
    // 会变成 ((size_t)&(((VertexData*)0)->VertexData::texcoord))
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, /*Stride*/sizeof(VertexData), (void*)offsetof(VertexData, VertexData::texcoord));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, /*Stride*/sizeof(VertexData), (void*)offsetof(VertexData, texcoord));

    // 状态机解绑
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

bool CubeRender::render(const RenderContext& context) {
    if ( !this->m_initialized ) {
        this->reportError(RenderError::InitializationFailed, "CubeRender Initialization failed");
    }

    glClearColor(this->m_clearColor.x, this->m_clearColor.y, this->m_clearColor.z, this->m_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 构建模型矩阵
    glm::mat4 modelMatrix = glm::mat4(1.0);
}


