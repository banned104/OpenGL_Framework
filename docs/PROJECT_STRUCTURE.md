# OpenGL 渲染框架项目文档

## 📁 项目结构

```
11_MainOpenGL/
├── 📄 CMakeLists.txt          # CMake 构建配置 (支持PC/Android)
├── 📄 main.cpp                # PC端应用程序入口 (Application类)
├── 📄 native_renderer.cpp     # Android JNI入口 (EGL管理)
├── 📄 compile_so.bat          # Android .so编译脚本
├── 📂 Component/              # 核心组件目录
│   ├── 📄 irenderer.hpp       # 渲染器接口定义
│   ├── 📄 render_config.hpp   # 渲染配置类 (含嵌入式shader)
│   ├── 📄 render_context.hpp  # 渲染上下文类
│   ├── 📄 render_factory.hpp  # 渲染器工厂
│   ├── 📄 shader.hpp/cpp      # Shader管理类
│   └── 📄 triangle_render.hpp/cpp  # 三角形渲染器实现
├── 📂 shaders/                # 着色器文件目录
│   ├── 📄 Convert_GLSL_to_h.py    # GLSL转头文件工具
│   ├── 📄 triangle.vert.glsl      # 顶点着色器源码
│   ├── 📄 triangle.frag.glsl      # 片段着色器源码
│   ├── 📄 triangle.vert.core.h    # PC版顶点着色器 (#version 330 core)
│   ├── 📄 triangle.frag.core.h    # PC版片段着色器
│   ├── 📄 triangle.vert.es.h      # Android版顶点着色器 (#version 310 es)
│   └── 📄 triangle.frag.es.h      # Android版片段着色器
├── 📂 example/                # 示例项目
│   └── 📂 android/            # Android示例工程
│       └── 📂 app/src/main/
│           ├── 📂 java/.../androidopengles/
│           │   ├── 📄 MainActivity.kt      # Android主Activity
│           │   ├── 📄 NativeRenderer.kt    # JNI桥接类
│           │   └── 📄 OpenGLSurfaceView.kt # OpenGL渲染视图
│           └── 📂 jniLibs/arm64-v8a/
│               └── 📄 libmain_opengl.so    # 编译后的Native库
├── 📂 3rdparty/               # 第三方库
│   ├── 📂 glad/               # OpenGL加载器 (仅PC)
│   ├── 📂 glfw/               # 窗口管理库 (仅PC)
│   └── 📂 glm/                # 数学库 (跨平台)
└── 📂 docs/                   # 文档目录
```

---

## 🌐 跨平台架构

### PC vs Android 对比

```mermaid
graph TB
    subgraph PC["🖥️ PC平台"]
        PC_MAIN[main.cpp<br/>Application类]
        PC_GLFW[GLFW<br/>窗口管理]
        PC_GLAD[GLAD<br/>OpenGL加载]
        PC_SHADER[triangle.*.core.h<br/>#version 330 core]
        PC_EXE[main_opengl.exe]
        
        PC_MAIN --> PC_GLFW
        PC_MAIN --> PC_GLAD
        PC_MAIN --> PC_SHADER
        PC_MAIN --> PC_EXE
    end
    
    subgraph Android["📱 Android平台"]
        AND_JNI[native_renderer.cpp<br/>JNI入口]
        AND_EGL[EGL<br/>上下文管理]
        AND_GLES[GLESv3<br/>OpenGL ES 3.x]
        AND_SHADER[triangle.*.es.h<br/>#version 310 es]
        AND_SO[libmain_opengl.so]
        AND_KT[Kotlin代码<br/>MainActivity等]
        
        AND_JNI --> AND_EGL
        AND_JNI --> AND_GLES
        AND_JNI --> AND_SHADER
        AND_JNI --> AND_SO
        AND_SO --> AND_KT
    end
    
    subgraph Shared["🔄 共享代码"]
        COMP[Component/<br/>渲染器核心]
        GLM[GLM<br/>数学库]
    end
    
    PC --> Shared
    Android --> Shared
```

### 条件编译机制

```mermaid
flowchart LR
    subgraph Source["源代码"]
        S1[shader.hpp]
        S2[triangle_render.hpp]
        S3[render_config.hpp]
    end
    
    subgraph Condition["#ifdef __ANDROID__"]
        C1{平台判断}
    end
    
    subgraph PC_Branch["PC分支"]
        P1["#include <glad/glad.h>"]
        P2["#include <triangle.vert.core.h>"]
    end
    
    subgraph Android_Branch["Android分支"]
        A1["#include <GLES3/gl3.h>"]
        A2["#include <triangle.vert.es.h>"]
    end
    
    Source --> C1
    C1 -->|PC| PC_Branch
    C1 -->|Android| Android_Branch
```

---

## 🏗️ 架构设计

### 整体架构图

```mermaid
graph TB
    subgraph Application["🖥️ Application Layer"]
        APP[Application类<br/>PC入口]
        JNI[native_renderer.cpp<br/>Android入口]
        MAIN[main.cpp]
    end
    
    subgraph Core["⚙️ Core Components"]
        IR[IRenderer<br/>渲染器接口]
        RF[RenderFactory<br/>渲染器工厂]
        RC[RenderConfig<br/>渲染配置]
        RX[RenderContext<br/>渲染上下文]
        SH[Shader<br/>着色器管理]
    end
    
    subgraph Renderers["🎨 Renderers"]
        TR[TriangleRender]
        CR[CubeRender<br/>可扩展]
        MR[MeshRender<br/>可扩展]
    end
    
    subgraph External["📦 3rdparty"]
        GLFW[GLFW<br/>窗口管理-PC]
        GLAD[GLAD<br/>OpenGL加载-PC]
        GLM[GLM<br/>数学运算]
        EGL[EGL<br/>上下文管理-Android]
        GLES[GLESv3<br/>OpenGL ES-Android]
    end
    
    MAIN --> APP
    JNI --> RF
    APP --> RF
    APP --> RC
    RF --> IR
    RF --> TR
    RF --> CR
    RF --> MR
    TR --> SH
    TR --> IR
    CR --> IR
    MR --> IR
    APP --> RX
    JNI --> RX
    TR --> RX
    APP --> GLFW
    APP --> GLAD
    JNI --> EGL
    JNI --> GLES
    SH --> GLM
    TR --> GLM
```

### 类关系图

```mermaid
classDiagram
    class IRenderer {
        <<interface>>
        +initialize(config: RenderConfig) bool
        +render(context: RenderContext) bool
        +resize(width: int, height: int) bool
        +cleanup() void
        +setErrorCallback(callback: ErrorCallback) void
        +getName() string
    }
    
    class TriangleRender {
        -m_shader: Shader
        -m_vao: GLuint
        -m_vbo: GLuint
        -m_projection: mat4
        -m_clearColor: vec4
        -m_rotationSpeed: float
        -m_currentAngle: float
        +initialize(config) bool
        +render(context) bool
        +resize(width, height) bool
        +cleanup() void
        -initializeGeometry(vertices) bool
        -reportError(error, message) void
    }
    
    class Shader {
        -m_programId: GLuint
        -m_uniformLocationCache: map
        -m_lastError: string
        +loadFromFile(vertexPath, fragmentPath) bool
        +loadFromSource(vertexSrc, fragmentSrc) bool
        +use() void
        +unuse() void
        +release() void
        +setMat4(name, mat) void
        +setVec3(name, vec) void
        +setFloat(name, value) void
        -compileShader(type, source) GLuint
        -linkProgram(vertex, fragment) bool
        -getUniformLocation(name) GLint
    }
    
    class RenderConfig {
        -m_vertexShaderSource: string
        -m_fragmentShaderSource: string
        -m_vertexData: vector~VertexData~
        -m_clearColor: vec4
        -m_rotationSpeed: float
        +setVertexShaderSource(src) RenderConfig
        +setFragmentShaderSource(src) RenderConfig
        +setVertexData(data) RenderConfig
        +setClearColor(r,g,b,a) RenderConfig
        +createTriangleConfig()$ RenderConfig
    }
    
    class RenderContext {
        -m_viewportSize: ViewportSize
        -m_projectionMatrix: mat4
        -m_deltaTime: float
        -m_frameNumber: uint64
        +viewportSize() ViewportSize
        +projectionMatrix() mat4
        +deltaTime() float
        +withFrameNumber(frame) RenderContext
        +withDeltaTime(dt) RenderContext
    }
    
    class RenderFactory {
        <<static>>
        +create(type: RenderType)$ unique_ptr~IRenderer~
        +create(typeName: string)$ unique_ptr~IRenderer~
    }
    
    class Application {
        -m_window: GLFWwindow*
        -m_renderer: unique_ptr~IRenderer~
        -m_config: RenderConfig
        -m_projectionMatrix: mat4
        +initialize() bool
        +run() void
        +shutdown() void
        -initializeGLFW() bool
        -initializeGLAD() bool
        -initializeRenderer() bool
        -render() void
        -onResize(width, height) void
    }
    
    IRenderer <|.. TriangleRender : implements
    TriangleRender --> Shader : uses
    TriangleRender --> RenderConfig : configured by
    TriangleRender --> RenderContext : receives
    RenderFactory --> IRenderer : creates
    Application --> RenderFactory : uses
    Application --> RenderConfig : creates
    Application --> RenderContext : creates
```

---

## 📱 Android JNI 架构

### JNI调用流程

```mermaid
sequenceDiagram
    participant KT as Kotlin代码
    participant JNI as JNI层
    participant EGL as EGL
    participant Renderer as TriangleRender
    
    Note over KT,Renderer: 初始化阶段 (在渲染线程中!)
    KT->>JNI: nativeInit(surface)
    JNI->>EGL: eglGetDisplay()
    JNI->>EGL: eglInitialize()
    JNI->>EGL: eglChooseConfig()
    JNI->>EGL: eglCreateWindowSurface()
    JNI->>EGL: eglCreateContext()
    JNI->>EGL: eglMakeCurrent()
    JNI->>Renderer: RenderFactory::create("triangle")
    JNI->>Renderer: initialize(config)
    JNI-->>KT: true
    
    Note over KT,Renderer: 渲染循环
    loop 每帧 (~60 FPS)
        KT->>JNI: nativeRender()
        JNI->>Renderer: render(context)
        JNI->>EGL: eglSwapBuffers()
    end
    
    Note over KT,Renderer: 清理阶段
    KT->>JNI: nativeCleanup()
    JNI->>Renderer: cleanup()
    JNI->>EGL: eglDestroyContext()
    JNI->>EGL: eglDestroySurface()
    JNI->>EGL: eglTerminate()
```

### Android线程模型

```mermaid
flowchart TB
    subgraph MainThread["主线程 (UI Thread)"]
        MT1[MainActivity.onCreate]
        MT2[surfaceCreated回调]
        MT3[surfaceChanged回调]
        MT4[surfaceDestroyed回调]
    end
    
    subgraph RenderThread["渲染线程 (OpenGL Thread)"]
        RT1[nativeInit<br/>创建EGL上下文]
        RT2[nativeResize<br/>更新视口]
        RT3[nativeRender<br/>渲染循环]
        RT4[nativeCleanup<br/>释放资源]
    end
    
    MT2 -->|启动线程| RT1
    MT3 -->|设置标志| RT2
    RT1 --> RT3
    RT3 -->|循环| RT3
    MT4 -->|停止标志| RT4
    
    style MainThread fill:#e3f2fd
    style RenderThread fill:#e8f5e9
    
    Note1[⚠️ 重要: EGL上下文是线程绑定的!<br/>必须在同一线程中创建和使用]
    RenderThread --> Note1
```

---

## 🔄 渲染流程

### 初始化流程

```mermaid
sequenceDiagram
    participant Main as main()
    participant App as Application
    participant GLFW as GLFW
    participant GLAD as GLAD
    participant Factory as RenderFactory
    participant Renderer as IRenderer
    participant Shader as Shader
    
    Main->>App: 创建Application(800, 600, "title")
    Main->>App: initialize()
    
    rect rgb(240, 248, 255)
        Note over App,GLFW: GLFW初始化
        App->>GLFW: glfwInit()
        App->>GLFW: glfwCreateWindow()
        App->>GLFW: glfwMakeContextCurrent()
        App->>GLFW: 设置回调函数
    end
    
    rect rgb(255, 248, 240)
        Note over App,GLAD: GLAD初始化
        App->>GLAD: gladLoadGLLoader()
    end
    
    rect rgb(240, 255, 240)
        Note over App,Shader: 渲染器初始化
        App->>Factory: create("triangle")
        Factory-->>App: unique_ptr<TriangleRender>
        App->>App: createTriangleConfig()
        App->>Renderer: initialize(config)
        Renderer->>Shader: loadFromSource(vert, frag)
        Shader->>Shader: compileShader()
        Shader->>Shader: linkProgram()
        Renderer->>Renderer: initializeGeometry()
    end
    
    App-->>Main: true
```

### 渲染循环流程

```mermaid
flowchart TD
    START([开始渲染循环]) --> CHECK{窗口关闭?}
    CHECK -->|否| INPUT[processInput<br/>处理输入]
    INPUT --> UPDATE[update<br/>更新逻辑]
    UPDATE --> RENDER[render<br/>渲染]
    
    subgraph RENDER_DETAIL["渲染详情"]
        R1[创建RenderContext] --> R2[设置帧号]
        R2 --> R3[调用renderer->render]
        R3 --> R4[清屏glClear]
        R4 --> R5[计算MVP矩阵]
        R5 --> R6[绑定Shader]
        R6 --> R7[设置Uniform]
        R7 --> R8[绑定VAO]
        R8 --> R9[glDrawArrays]
    end
    
    RENDER --> SWAP[glfwSwapBuffers<br/>交换缓冲区]
    SWAP --> POLL[glfwPollEvents<br/>轮询事件]
    POLL --> FPS[updateFPS<br/>更新FPS]
    FPS --> CHECK
    
    CHECK -->|是| CLEANUP[shutdown<br/>清理资源]
    CLEANUP --> END([结束])
```

---

## 🛠️ 构建系统

### Shader编译流程

```mermaid
flowchart LR
    subgraph Input["输入文件"]
        VERT[triangle.vert.glsl]
        FRAG[triangle.frag.glsl]
    end
    
    subgraph Tool["转换工具"]
        PY[Convert_GLSL_to_h.py]
    end
    
    subgraph PC_Output["PC输出 (--pc)"]
        PC_V[triangle.vert.core.h<br/>#version 330 core]
        PC_F[triangle.frag.core.h]
    end
    
    subgraph Android_Output["Android输出 (--android)"]
        AND_V[triangle.vert.es.h<br/>#version 310 es<br/>precision highp float]
        AND_F[triangle.frag.es.h]
    end
    
    VERT --> PY
    FRAG --> PY
    PY -->|--pc| PC_Output
    PY -->|--android| Android_Output
```

### Android编译流程 (compile_so.bat)

```mermaid
flowchart TD
    START([compile_so.bat]) --> STEP1
    
    STEP1["[1/4] 转换Shader<br/>python Convert_GLSL_to_h.py --android"]
    STEP1 --> STEP2
    
    STEP2["[2/4] 配置CMake<br/>cmake -G Ninja<br/>-DCMAKE_TOOLCHAIN_FILE=android.toolchain.cmake<br/>-DANDROID_ABI=arm64-v8a<br/>-DBUILD_AS_SHARED=ON"]
    STEP2 --> STEP3
    
    STEP3["[3/4] 编译<br/>ninja"]
    STEP3 --> STEP4
    
    STEP4["[4/4] 复制.so<br/>copy libmain_opengl.so<br/>→ jniLibs/arm64-v8a/"]
    STEP4 --> END([完成])
    
    style STEP1 fill:#e3f2fd
    style STEP2 fill:#fff3e0
    style STEP3 fill:#e8f5e9
    style STEP4 fill:#fce4ec
```

### CMakeLists.txt 条件编译

```mermaid
flowchart TB
    subgraph CMake["CMakeLists.txt"]
        CHECK{ANDROID OR<br/>BUILD_AS_SHARED?}
    end
    
    subgraph Android_Build["Android构建"]
        A1[add_library SHARED]
        A2[target_link_libraries:<br/>GLESv3, EGL, android, log]
        A3[输出: libmain_opengl.so]
    end
    
    subgraph PC_Build["PC构建"]
        P1[add_executable]
        P2[target_link_libraries:<br/>glfw, glad, OpenGL]
        P3[输出: main_opengl.exe]
    end
    
    CHECK -->|是| Android_Build
    CHECK -->|否| PC_Build
```

---

## 🆕 创建新渲染器指南

### 步骤概览

```mermaid
flowchart LR
    A[1. 创建头文件] --> B[2. 实现渲染器类]
    B --> C[3. 编写着色器]
    C --> D[4. 注册到工厂]
    D --> E[5. 创建配置方法]
    E --> F[6. 更新CMake]
    
    style A fill:#e1f5fe
    style B fill:#e8f5e9
    style C fill:#fff3e0
    style D fill:#fce4ec
    style E fill:#f3e5f5
    style F fill:#e0f2f1
```

### 详细步骤

#### 步骤 1: 创建渲染器头文件

在 `Component/` 目录下创建新文件，例如 `cube_render.hpp`:

```cpp
#pragma once
#include "irenderer.hpp"
#include "render_config.hpp"
#include "render_context.hpp"
#include "shader.hpp"

// 跨平台OpenGL头文件
#ifdef __ANDROID__
    #include <GLES3/gl3.h>
#else
    #include <glad/glad.h>
#endif

#include <glm/glm.hpp>

class CubeRender : public IRenderer {
public:
    CubeRender();
    ~CubeRender() override;

    // 实现IRenderer接口
    bool initialize(const RenderConfig& config) override;
    bool render(const RenderContext& context) override;
    bool resize(int width, int height) override;
    void cleanup() override;
    void setErrorCallback(ErrorCallback callback) override;
    std::string getName() const override { return "CubeRender"; }

private:
    bool initializeGeometry(const std::vector<VertexData>& vertices);
    void reportError(RenderError error, const std::string& message);

    Shader m_shader;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;  // 索引缓冲
    glm::mat4 m_projection;
    glm::vec4 m_clearColor;
    
    ErrorCallback m_errorCallback;
    bool m_initialized;
};
```

#### 步骤 2: 实现渲染器

创建 `cube_render.cpp`:

```cpp
#include "cube_render.hpp"
#include <iostream>

CubeRender::CubeRender()
    : m_vao(0), m_vbo(0), m_ebo(0), m_initialized(false) {}

CubeRender::~CubeRender() { cleanup(); }

bool CubeRender::initialize(const RenderConfig& config) {
    // 1. 从源码加载着色器（编译时嵌入）
    if (!m_shader.loadFromSource(config.vertexShaderSource(), 
                                  config.fragmentShaderSource())) {
        reportError(RenderError::ShaderCompilationFailed, 
                   m_shader.lastError());
        return false;
    }

    // 2. 初始化几何体
    if (!initializeGeometry(config.vertexData())) {
        reportError(RenderError::BufferCreationFailed, 
                   "Failed to create buffers");
        return false;
    }

    m_clearColor = config.clearColor();
    m_initialized = true;
    return true;
}

bool CubeRender::render(const RenderContext& context) {
    if (!m_initialized) return false;

    glClearColor(m_clearColor.x, m_clearColor.y, 
                 m_clearColor.z, m_clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 计算MVP矩阵
    glm::mat4 model = glm::mat4(1.0f);
    // ... 添加变换 ...
    glm::mat4 mvp = context.projectionMatrix() * model;

    // 渲染
    m_shader.use();
    m_shader.setMat4("mvp", mvp);
    
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    return true;
}

// ... 其他方法实现 ...
```

#### 步骤 3: 编写着色器

在 `shaders/` 目录下创建着色器文件，然后使用转换工具:

**cube.vert.glsl:**
```glsl
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

out vec3 fragColor;
uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    fragColor = color;
}
```

**生成头文件:**
```bash
# PC版本
python shaders/Convert_GLSL_to_h.py shaders/cube.vert.glsl shaders/cube.vert.core.h --pc

# Android版本
python shaders/Convert_GLSL_to_h.py shaders/cube.vert.glsl shaders/cube.vert.es.h --android
```

#### 步骤 4: 注册到工厂

修改 `render_factory.hpp`:

```cpp
#pragma once
#include "irenderer.hpp"
#include "triangle_render.hpp"
#include "cube_render.hpp"  // 添加新渲染器头文件
#include <memory>

enum class RenderType {
    Triangle,
    Cube,      // 添加新类型
    Custom,
};

class RenderFactory {
public:
    static std::unique_ptr<IRenderer> create(RenderType type) {
        switch (type) {
        case RenderType::Triangle:
            return std::make_unique<TriangleRender>();
        case RenderType::Cube:                              // 新增
            return std::make_unique<CubeRender>();          // 新增
        default:
            return nullptr;
        }
    }

    static std::unique_ptr<IRenderer> create(const std::string& typeName) {
        if (typeName == "triangle") {
            return create(RenderType::Triangle);
        } else if (typeName == "cube") {                    // 新增
            return create(RenderType::Cube);                // 新增
        }
        return nullptr;
    }
};
```

#### 步骤 5: 添加配置方法

在 `render_config.hpp` 中添加:

```cpp
// 在头文件顶部添加条件包含
#ifdef __ANDROID__
    #include <cube.vert.es.h>
    #include <cube.frag.es.h>
#else
    #include <cube.vert.core.h>
    #include <cube.frag.core.h>
#endif

// 添加配置方法
static RenderConfig createCubeConfig() {
    RenderConfig config;
    
    // 使用编译时嵌入的着色器
    config.setVertexShaderSource(CUBE_VERTEX_SHADER)
          .setFragmentShaderSource(CUBE_FRAGMENT_SHADER);
    
    // 设置立方体顶点数据
    std::vector<VertexData> vertices = {
        // ... 立方体顶点 ...
    };
    
    config.setVertexData(vertices)
          .setClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    
    return config;
}
```

#### 步骤 6: 更新CMakeLists.txt

```cmake
set(COMPONENT_SOURCES
    Component/triangle_render.cpp
    Component/cube_render.cpp      # 添加新文件
    Component/shader.cpp
)

set(COMPONENT_HEADERS
    Component/irenderer.hpp
    Component/render_config.hpp
    Component/render_context.hpp
    Component/render_factory.hpp
    Component/triangle_render.hpp
    Component/cube_render.hpp      # 添加新文件
    Component/shader.hpp
)
```

---

## 🎯 设计模式说明

```mermaid
mindmap
  root((设计模式))
    工厂模式
      RenderFactory
      根据类型创建渲染器
      解耦创建和使用
    策略模式
      IRenderer接口
      不同渲染策略
      运行时切换
    Builder模式
      RenderConfig
      链式调用
      灵活配置
    RAII
      Shader类
      自动资源管理
      异常安全
    单例模式
      Application
      唯一窗口实例
      全局访问点
```

---

## 📝 关键代码示例

### 使用新渲染器

```cpp
// 在Application中切换渲染器
bool Application::initializeRenderer() {
    // 方式1: 使用字符串
    m_renderer = RenderFactory::create("cube");
    
    // 方式2: 使用枚举
    m_renderer = RenderFactory::create(RenderType::Cube);
    
    // 使用对应配置
    m_config = RenderConfig::createCubeConfig();
    
    return m_renderer->initialize(m_config);
}
```

### Shader类使用示例

```cpp
Shader shader;

// 从源码加载（推荐 - 编译时嵌入）
if (shader.loadFromSource(VERTEX_SHADER_SOURCE, FRAGMENT_SHADER_SOURCE)) {
    shader.use();
    
    // 设置uniform变量
    shader.setMat4("mvp", mvpMatrix);
    shader.setVec3("lightPos", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setFloat("time", currentTime);
    
    // 渲染...
    
    shader.unuse();
}

// 或从文件加载（仅PC调试用）
shader.loadFromFile("vertex.glsl", "fragment.glsl");
```

---

## 🔧 扩展建议

1. **添加纹理支持**: 创建Texture类管理纹理加载
2. **添加模型加载**: 集成Assimp库加载3D模型
3. **添加光照系统**: 实现Phong/PBR光照
4. **添加相机系统**: 创建Camera类管理视图变换
5. **添加ImGui**: 集成调试界面
6. **多ABI支持**: 添加armeabi-v7a, x86_64等架构

```mermaid
graph LR
    subgraph Future["未来扩展"]
        TEX[Texture类]
        CAM[Camera类]
        LIGHT[Light类]
        MODEL[ModelLoader类]
        UI[ImGui集成]
        ABI[多ABI支持]
    end
    
    subgraph Current["当前架构"]
        IR[IRenderer]
        SH[Shader]
        APP[Application]
        JNI[JNI Bridge]
    end
    
    Current --> Future
```

---

## ⚠️ 注意事项

### Android开发关键点

1. **EGL上下文线程绑定**: EGL上下文只能在创建它的线程中使用，必须确保`nativeInit()`、`nativeRender()`、`nativeCleanup()`在同一线程调用

2. **Shader版本差异**: 
   - PC: `#version 330 core`
   - Android: `#version 310 es` + `precision highp float;`

3. **库命名**: .so文件必须以`lib`开头，加载时去掉前缀
   - 文件名: `libmain_opengl.so`
   - 加载: `System.loadLibrary("main_opengl")`

4. **JNI函数命名**: 必须严格匹配包名
   - 格式: `Java_包名_类名_方法名`
   - 示例: `Java_com_example_androidopengles_NativeRenderer_nativeInit`
