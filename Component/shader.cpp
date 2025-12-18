#include "shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader()
    : m_programId(0)
{
}

Shader::~Shader() {
    release();
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(other.m_programId)
    , m_uniformLocationCache(std::move(other.m_uniformLocationCache))
    , m_lastError(std::move(other.m_lastError))
{
    other.m_programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        release();
        m_programId = other.m_programId;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        m_lastError = std::move(other.m_lastError);
        other.m_programId = 0;
    }
    return *this;
}

bool Shader::loadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource = readFile(vertexPath);
    std::string fragmentSource = readFile(fragmentPath);

    if (vertexSource.empty()) {
        m_lastError = "Failed to read vertex shader file: " + vertexPath;
        return false;
    }

    if (fragmentSource.empty()) {
        m_lastError = "Failed to read fragment shader file: " + fragmentPath;
        return false;
    }

    return loadFromSource(vertexSource, fragmentSource);
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    // 先释放旧的程序
    release();

    // 编译顶点着色器
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return false;
    }

    // 编译片段着色器
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // 链接程序
    bool success = linkProgram(vertexShader, fragmentShader);

    // 删除着色器对象（已链接到程序中）
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return success;
}

void Shader::use() const {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void Shader::unuse() const {
    glUseProgram(0);
}

void Shader::release() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
    m_uniformLocationCache.clear();
}

// ============ Uniform 设置方法实现 ============

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec2(const std::string& name, float x, float y) const {
    glUniform2f(getUniformLocation(name), x, y);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat) const {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

// ============ 私有方法实现 ============

std::string Shader::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Shader: Failed to open file: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // 检查编译错误
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        
        const char* typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        m_lastError = std::string(typeStr) + " shader compilation failed: " + infoLog;
        std::cerr << "Shader: " << m_lastError << std::endl;
        
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Shader::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertexShader);
    glAttachShader(m_programId, fragmentShader);
    glLinkProgram(m_programId);

    // 检查链接错误
    GLint success;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);
        
        m_lastError = std::string("Shader program linking failed: ") + infoLog;
        std::cerr << "Shader: " << m_lastError << std::endl;
        
        glDeleteProgram(m_programId);
        m_programId = 0;
        return false;
    }

    return true;
}

GLint Shader::getUniformLocation(const std::string& name) const {
    // 先查缓存
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    // 查询并缓存
    GLint location = glGetUniformLocation(m_programId, name.c_str());
    if (location == -1) {
        std::cerr << "Shader: Warning - uniform '" << name << "' not found or not used" << std::endl;
    }
    m_uniformLocationCache[name] = location;
    return location;
}
