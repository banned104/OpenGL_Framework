// IRenderConfig.hpp
// 单一职责: 定义渲染器配置的抽象接口
#pragma once
#include <string>
#include <glm/glm.hpp>

class IRenderConfig {
public:
    virtual ~IRenderConfig() = default;

    // 着色器源码
    virtual const std::string& vertexShaderSource() const = 0;
    virtual const std::string& fragmentShaderSource() const = 0;

    // 渲染参数
    virtual glm::vec4 clearColor() const = 0;
    virtual float rotationSpeed() const = 0;

    // 顶点数据访问 (通过 void* 实现类型擦除)
    virtual const void* vertexData() const = 0;
    virtual size_t vertexCount() const = 0;
    virtual size_t vertexStride() const = 0;
};
