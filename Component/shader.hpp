#pragma once

// 平台条件编译：OpenGL ES (Android) vs OpenGL Core (PC)
#ifdef __ANDROID__
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>
#else
    #include <glad/glad.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <unordered_map>

/**
 * @brief Shader类 - 封装OpenGL着色器程序的加载、编译和使用
 * 
 * 单一职责: 管理着色器的生命周期和uniform变量设置
 */
class Shader {
public:
    Shader();
    ~Shader();

    // 禁止拷贝，允许移动
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief 从文件加载并编译着色器
     * @param vertexPath 顶点着色器文件路径
     * @param fragmentPath 片段着色器文件路径
     * @return 是否成功
     */
    bool loadFromFile(const std::string& vertexPath, const std::string& fragmentPath);

    /**
     * @brief 从源码字符串编译着色器
     * @param vertexSource 顶点着色器源码
     * @param fragmentSource 片段着色器源码
     * @return 是否成功
     */
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

    /**
     * @brief 激活着色器程序
     */
    void use() const;

    /**
     * @brief 解绑着色器程序
     */
    void unuse() const;

    /**
     * @brief 获取着色器程序ID
     */
    GLuint programId() const { return m_programId; }

    /**
     * @brief 检查着色器是否有效
     */
    bool isValid() const { return m_programId != 0; }

    /**
     * @brief 释放着色器资源
     */
    void release();

    // ============ Uniform 设置方法 ============

    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    /**
     * @brief 获取最后的错误信息
     */
    std::string lastError() const { return m_lastError; }

private:
    /**
     * @brief 从文件读取着色器源码
     */
    std::string readFile(const std::string& filepath);

    /**
     * @brief 编译单个着色器
     */
    GLuint compileShader(GLenum type, const std::string& source);

    /**
     * @brief 链接着色器程序
     */
    bool linkProgram(GLuint vertexShader, GLuint fragmentShader);

    /**
     * @brief 获取uniform位置（带缓存）
     */
    GLint getUniformLocation(const std::string& name) const;

private:
    GLuint m_programId;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;
    std::string m_lastError;
};
