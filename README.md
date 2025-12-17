# OpenGL 项目

这是一个使用纯C++和OpenGL的渲染项目，已移除所有Qt依赖。

## 依赖库

- GLFW: 窗口管理和输入处理
- GLAD: OpenGL函数加载器
- GLM: OpenGL数学库

## 项目结构

```
├── Component/          # 渲染组件
│   ├── irenderer.hpp          # 渲染器接口
│   ├── render_config.hpp      # 渲染配置
│   ├── render_context.hpp     # 渲染上下文
│   ├── render_factory.hpp     # 渲染器工厂
│   ├── triangle_render.hpp    # 三角形渲染器头文件
│   └── triangle_render.cpp    # 三角形渲染器实现
├── shaders/           # 着色器文件
│   ├── triangle.vert  # 顶点着色器
│   └── triangle.frag  # 片段着色器
├── 3rdparty/          # 第三方库
│   ├── glfw/
│   ├── glad/
│   └── glm/
├── CMakeLists.txt     # CMake配置
└── main.cpp          # 主程序入口
```

## 构建项目

### Windows (使用Visual Studio)

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make
```

## 运行

构建完成后，可执行文件位于：
- Windows: `build/Release/main_opengl.exe`
- Linux/macOS: `build/main_opengl`

运行程序将打开一个窗口，显示一个旋转的彩色三角形。

## 按键控制

- `ESC`: 退出程序

## 主要变更（相比Qt版本）

1. **移除Qt依赖**：
   - `QString` → `std::string`
   - `QVector3D/QVector4D` → `glm::vec3/glm::vec4`
   - `QMatrix4x4` → `glm::mat4`
   - `QSize` → 自定义 `ViewportSize` 结构

2. **OpenGL函数调用**：
   - `QOpenGLFunctions` → GLAD直接调用OpenGL函数
   - `QOpenGLBuffer` → 手动管理VAO/VBO
   - `QOpenGLShaderProgram` → 手动加载和编译着色器

3. **窗口管理**：
   - `QQuickFramebufferObject` → GLFW窗口

## 注意事项

- 确保 `shaders/` 目录与可执行文件在同一目录下，或使用绝对路径
- 需要OpenGL 3.3或更高版本支持
