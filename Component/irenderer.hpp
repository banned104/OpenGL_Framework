// IRenderer.h
// 单一职责: 定义所有渲染器必须实现的标准接口
#pragma once
#include <memory>
#include <functional>
#include <string>

// 前向声明
class RenderContext;
class RenderConfig;

enum class RenderError {
    None = 0,
    InitializationFailed,
    ShaderCompilationFailed,
    BufferCreationFailed,
    RenderingFailed
};

// 回调函数类型定义
using ErrorCallback = std::function<void(RenderError, const std::string&)>;
using StateCallback = std::function<void(const std::string&)>;

class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // 初始化渲染器
    virtual bool initialize(const RenderConfig& config) = 0;
    
    // 执行渲染
    virtual bool render(const RenderContext& context) = 0;
    
    // 调整视口大小
    virtual bool resize(int width, int height) = 0;

    
    // 清理资源
    virtual void cleanup() = 0;
    
    // 设置错误回调 不使用 Q_OBJECT 降低对Qt的依赖
    virtual void setErrorCallback(ErrorCallback callback) = 0;
    
    // 获取渲染器名称（用于调试）
    virtual std::string getName() const = 0;
};
