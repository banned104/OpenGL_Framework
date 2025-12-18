/**
 * @file native_renderer.cpp
 * @brief Android JNI入口 - 提供给Java/Kotlin调用的Native渲染接口
 * 
 * ============================================================================
 * 设计特点：完全适合作为Android控件使用
 * ============================================================================
 * 
 * 1. 独立性：每个OpenGLSurfaceView实例都有自己的Surface，可以在同一Activity中
 *    创建多个OpenGL控件，它们不会互相干扰
 * 
 * 2. 灵活布局：OpenGLSurfaceView继承自SurfaceView，可以像普通View一样：
 *    - 在XML布局中定义大小和位置
 *    - 与Button、TextView等其他控件混合使用
 *    - 支持ConstraintLayout、LinearLayout等任意布局
 *    - 可以设置margin、padding等布局属性
 * 
 * 3. 生命周期管理：通过SurfaceHolder.Callback自动管理：
 *    - surfaceCreated: OpenGL上下文创建（用户看到控件时）
 *    - surfaceDestroyed: OpenGL资源释放（控件销毁或进入后台时）
 *    - 无需手动管理Activity生命周期
 * 
 * 4. 多实例支持（当前为单例，可扩展）：
 *    - 当前使用全局变量，支持单个OpenGL控件
 *    - 如需多个控件，可改为map<jobject, RendererState>管理多实例
 * 
 * 5. 线程安全：每个OpenGL上下文在独立的渲染线程中运行，不阻塞UI线程
 * 
 * ============================================================================
 * 使用示例：在布局中嵌入OpenGL控件
 * ============================================================================
 * 
 * XML布局示例：
 * <LinearLayout>
 *     <TextView 
 *         android:text="OpenGL渲染演示"
 *         android:layout_height="wrap_content"/>
 *     
 *     <!-- OpenGL控件，只占据部分屏幕 -->
 *     <com.example.androidopengles.OpenGLSurfaceView
 *         android:layout_width="match_parent"
 *         android:layout_height="300dp"/>  <!-- 固定高度300dp -->
 *     
 *     <Button
 *         android:text="切换场景"
 *         android:layout_height="wrap_content"/>
 * </LinearLayout>
 * 
 * ============================================================================
 */

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>        // ANativeWindow相关API
#include <android/native_window_jni.h>    // Surface转ANativeWindow

#include <EGL/egl.h>     // EGL API - OpenGL ES与窗口系统的桥梁
#include <GLES3/gl3.h>   // OpenGL ES 3.0 API

#include <memory>
#include <string>

#include "render_factory.hpp"   // 渲染器工厂
#include "render_config.hpp"    // 渲染配置
#include "render_context.hpp"   // 渲染上下文

// Android日志宏定义
#define LOG_TAG "NativeRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// 全局渲染器状态
// ============================================================================
// 
// 注意：当前使用全局变量，支持单个OpenGL控件实例
// 
// 如果需要在同一Activity中使用多个OpenGL控件，需要改为：
// std::map<jobject, std::unique_ptr<RendererState>> g_renderers;
// 
// 这样每个Java对象(OpenGLSurfaceView实例)都有独立的渲染状态
// ============================================================================

namespace {
    // ------------------------------------------------------------
    // EGL相关资源
    // ------------------------------------------------------------
    // EGL (Embedded-System Graphics Library) 是OpenGL ES与原生窗口系统的接口
    
    EGLDisplay g_display = EGL_NO_DISPLAY;  // EGL显示连接（通常是屏幕）
    EGLSurface g_surface = EGL_NO_SURFACE;  // EGL绘图表面（关联到Android Surface）
    EGLContext g_context = EGL_NO_CONTEXT;  // OpenGL ES渲染上下文（存储OpenGL状态）
    ANativeWindow* g_window = nullptr;       // Android原生窗口（从Java Surface获取）
    
    // ------------------------------------------------------------
    // 渲染器资源
    // ------------------------------------------------------------
    std::unique_ptr<IRenderer> g_renderer;   // 渲染器实例（如TriangleRender）
    RenderConfig g_config;                   // 渲染配置（shader、顶点数据等）
    glm::mat4 g_projectionMatrix(1.0f);      // 投影矩阵（透视或正交）
    
    // ------------------------------------------------------------
    // 视口状态
    // ------------------------------------------------------------
    int g_width = 0;                // 控件宽度（像素）
    int g_height = 0;               // 控件高度（像素）
    uint64_t g_frameNumber = 0;     // 当前帧号（用于动画）
    
    // ------------------------------------------------------------
    // 初始化标志
    // ------------------------------------------------------------
    bool g_initialized = false;     // 是否已初始化（防止重复初始化）
}

// ============================================================================
// EGL初始化和清理
// ============================================================================
// 
// EGL (Embedded-System Graphics Library) 作用：
// 1. 连接OpenGL ES与Android窗口系统
// 2. 管理OpenGL ES的渲染上下文
// 3. 控制双缓冲和垂直同步
// 
// 为什么适合作为控件：
// - 每个SurfaceView都有独立的Surface
// - 每个Surface可以创建独立的EGLSurface
// - 即使在同一Activity中有多个OpenGL控件，它们的Surface也是隔离的
// ============================================================================

/**
 * @brief 初始化EGL环境
 * 
 * 完整的EGL初始化流程，创建OpenGL ES 3.0渲染环境
 * 
 * @param window ANativeWindow指针（从Java Surface获取）
 * @return true 初始化成功，false 失败
 * 
 * @note 此函数必须在渲染线程中调用！
 *       EGL上下文与线程绑定，后续所有OpenGL调用必须在同一线程
 */
static bool initEGL(ANativeWindow* window) {
    // ------------------------------------------------------------------------
    // 步骤1: 获取EGL显示连接
    // ------------------------------------------------------------------------
    // EGL_DEFAULT_DISPLAY 表示使用默认显示设备（通常是主屏幕）
    // 如果设备有多个屏幕，可以指定具体的显示ID
    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤2: 初始化EGL
    // ------------------------------------------------------------------------
    // 初始化EGL库，返回EGL版本信息
    EGLint majorVersion, minorVersion;
    if (!eglInitialize(g_display, &majorVersion, &minorVersion)) {
        LOGE("eglInitialize failed");
        return false;
    }
    LOGI("EGL version: %d.%d", majorVersion, minorVersion);
    
    // ------------------------------------------------------------------------
    // 步骤3: 配置EGL参数
    // ------------------------------------------------------------------------
    // 这些参数定义了我们需要的帧缓冲格式
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,        // 渲染到窗口（而非离屏缓冲）
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, // 使用OpenGL ES 3.0
        EGL_RED_SIZE, 8,                         // 红色通道8位
        EGL_GREEN_SIZE, 8,                       // 绿色通道8位
        EGL_BLUE_SIZE, 8,                        // 蓝色通道8位
        EGL_ALPHA_SIZE, 8,                       // Alpha通道8位（支持透明）
        EGL_DEPTH_SIZE, 24,                      // 深度缓冲24位（3D渲染必需）
        EGL_STENCIL_SIZE, 8,                     // 模板缓冲8位（高级效果如阴影）
        EGL_NONE                                 // 数组结束标记
    };
    
    // 选择最匹配的EGL配置
    // 系统可能返回多个配置，我们只取第一个
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_display, configAttribs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("eglChooseConfig failed");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤4: 创建EGL窗口Surface
    // ------------------------------------------------------------------------
    // 将EGL绑定到Android的原生窗口
    // 这是OpenGL内容最终显示的目标
    // 
    // 关键：每个SurfaceView都有独立的ANativeWindow
    // 所以多个OpenGL控件不会互相干扰
    g_surface = eglCreateWindowSurface(g_display, config, window, nullptr);
    if (g_surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤5: 创建OpenGL ES渲染上下文
    // ------------------------------------------------------------------------
    // 上下文包含所有OpenGL状态（着色器、缓冲区、纹理等）
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,  // 请求OpenGL ES 3.0
        EGL_NONE
    };
    
    // EGL_NO_CONTEXT 表示不与其他上下文共享资源
    // 如果多个OpenGL控件需要共享纹理/缓冲区，可以传入已有上下文
    g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, contextAttribs);
    if (g_context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤6: 绑定上下文到当前线程
    // ------------------------------------------------------------------------
    // 关键操作！将OpenGL上下文绑定到当前线程
    // 之后在这个线程的所有OpenGL调用都会影响这个上下文
    // 
    // 参数说明：
    // - g_display: EGL显示连接
    // - g_surface: 用于绘制的Surface
    // - g_surface: 用于读取的Surface（通常与绘制Surface相同）
    // - g_context: OpenGL ES上下文
    if (!eglMakeCurrent(g_display, g_surface, g_surface, g_context)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤7: 查询Surface信息
    // ------------------------------------------------------------------------
    // 获取控件的实际像素尺寸
    // 注意：这个尺寸由XML布局中的layout_width/layout_height决定
    eglQuerySurface(g_display, g_surface, EGL_WIDTH, &g_width);
    eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &g_height);
    LOGI("Surface size: %dx%d", g_width, g_height);
    
    // ------------------------------------------------------------------------
    // 步骤8: 打印OpenGL ES设备信息
    // ------------------------------------------------------------------------
    // 这些信息对调试很有用
    LOGI("GL_VENDOR: %s", glGetString(GL_VENDOR));       // 厂商：如Qualcomm、ARM
    LOGI("GL_RENDERER: %s", glGetString(GL_RENDERER));   // GPU型号：如Adreno 740
    LOGI("GL_VERSION: %s", glGetString(GL_VERSION));     // OpenGL ES版本
    
    return true;
}

/**
 * @brief 清理EGL资源
 * 
 * 按照正确的顺序释放EGL资源，防止内存泄漏
 * 
 * 释放顺序很重要：
 * 1. 解绑上下文（让当前线程不再使用OpenGL）
 * 2. 销毁上下文（释放OpenGL状态）
 * 3. 销毁Surface（释放渲染目标）
 * 4. 终止EGL连接
 * 
 * @note 必须在创建EGL的同一线程中调用
 */
static void terminateEGL() {
    if (g_display != EGL_NO_DISPLAY) {
        // 步骤1: 解绑当前上下文
        // 将空值绑定到当前线程，释放OpenGL资源的独占权
        eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        // 步骤2: 销毁OpenGL ES上下文
        if (g_context != EGL_NO_CONTEXT) {
            eglDestroyContext(g_display, g_context);
            g_context = EGL_NO_CONTEXT;
        }
        
        // 步骤3: 销毁EGL Surface
        if (g_surface != EGL_NO_SURFACE) {
            eglDestroySurface(g_display, g_surface);
            g_surface = EGL_NO_SURFACE;
        }
        
        // 步骤4: 终止EGL显示连接
        eglTerminate(g_display);
        g_display = EGL_NO_DISPLAY;
    }
}

// ============================================================================
// 渲染器初始化和清理
// ============================================================================
// 
// 这部分代码负责管理我们的渲染器（TriangleRender等）
// 与EGL初始化分离，保持职责单一
// ============================================================================

/**
 * @brief 初始化渲染器
 * 
 * 创建并配置渲染器实例：
 * 1. 使用工厂模式创建渲染器
 * 2. 加载并编译shader
 * 3. 创建顶点缓冲区(VBO/VAO)
 * 4. 设置投影矩阵
 * 
 * @return true 初始化成功，false 失败
 * 
 * @note 此函数在EGL上下文创建后调用，所有OpenGL调用都是有效的
 */
static bool initRenderer() {
    // ------------------------------------------------------------------------
    // 步骤1: 创建渲染器实例
    // ------------------------------------------------------------------------
    // 使用工厂模式，方便切换不同的渲染器
    // "triangle" -> TriangleRender
    // "cube"     -> CubeRender (如果实现了)
    g_renderer = RenderFactory::create("triangle");
    if (!g_renderer) {
        LOGE("Failed to create renderer");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤2: 设置错误回调
    // ------------------------------------------------------------------------
    // 使用Lambda表达式，将渲染器的错误信息输出到Android日志
    // 这样在Logcat中可以看到详细的错误信息
    g_renderer->setErrorCallback([](RenderError error, const std::string& msg) {
        LOGE("Render Error [%d]: %s", static_cast<int>(error), msg.c_str());
    });
    
    // ------------------------------------------------------------------------
    // 步骤3: 创建渲染配置
    // ------------------------------------------------------------------------
    // RenderConfig包含：
    // - Shader源码（编译时嵌入的头文件）
    // - 顶点数据（位置、颜色等）
    // - 清屏颜色
    // - 旋转速度等参数
    g_config = RenderConfig::createTriangleConfig();
    
    // ------------------------------------------------------------------------
    // 步骤4: 初始化渲染器
    // ------------------------------------------------------------------------
    // 这会执行：
    // - 编译和链接shader程序
    // - 创建VAO/VBO
    // - 上传顶点数据到GPU
    if (!g_renderer->initialize(g_config)) {
        LOGE("Failed to initialize renderer");
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 步骤5: 设置视口和投影矩阵
    // ------------------------------------------------------------------------
    // 视口(Viewport)：定义OpenGL渲染区域的像素坐标
    // 投影矩阵：将3D场景投影到2D屏幕
    g_renderer->resize(g_width, g_height);
    
    // 计算宽高比，避免图像变形
    // aspect = 宽/高，例如 1920/1080 = 1.78 (16:9)
    float aspect = static_cast<float>(g_width) / static_cast<float>(g_height);
    
    // 创建透视投影矩阵
    // 参数：
    // - 30度视场角(FOV)：控制"缩放"程度
    // - aspect: 宽高比
    // - 3.0f: 近裁剪面（小于此距离的物体不渲染）
    // - 10.0f: 远裁剪面（大于此距离的物体不渲染）
    g_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
    
    LOGI("Renderer initialized successfully");
    return true;
}

/**
 * @brief 清理渲染器资源
 * 
 * 释放渲染器占用的GPU资源：
 * - VAO/VBO（顶点缓冲区）
 * - Shader程序
 * - 纹理等
 * 
 * @note 必须在OpenGL上下文有效时调用
 */
static void cleanupRenderer() {
    if (g_renderer) {
        g_renderer->cleanup();  // 释放OpenGL资源
        g_renderer.reset();      // 释放C++对象
    }
}

// ============================================================================
// JNI导出函数
// ============================================================================
// 
// 这些函数导出给Java/Kotlin调用，函数名必须严格遵循JNI命名规则：
// Java_<包名>_<类名>_<方法名>
// 
// 包名中的点(.)要替换为下划线(_)
// 例如：com.example.androidopengles -> com_example_androidopengles
// 
// 作为控件的优势：
// - 这些函数只管理OpenGL渲染，不涉及Activity生命周期
// - OpenGLSurfaceView自动处理Surface的创建/销毁
// - 可以在任何布局中使用，不需要全屏
// ============================================================================

extern "C" {

/**
 * @brief 初始化OpenGL渲染环境
 * 
 * Kotlin调用示例：
 *   val renderer = NativeRenderer()
 *   renderer.nativeInit(holder.surface)
 * 
 * 时机：在OpenGLSurfaceView的渲染线程中调用
 * 
 * 完整流程：
 * 1. 检查是否已初始化（防止重复初始化）
 * 2. 从Java Surface获取ANativeWindow
 * 3. 初始化EGL（创建OpenGL上下文）
 * 4. 初始化渲染器（加载shader、创建缓冲区）
 * 
 * @param env JNI环境指针（每个JNI调用都需要）
 * @param thiz Java对象引用（NativeRenderer实例）
 * @param surface Java Surface对象（从SurfaceView.getHolder().getSurface()获取）
 * @return JNI_TRUE 初始化成功，JNI_FALSE 失败
 * 
 * @note 关键：必须在渲染线程中调用，不能在主线程！
 *       否则EGL上下文会绑定到错误的线程
 */
JNIEXPORT jboolean JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeInit(JNIEnv* env, jobject thiz, jobject surface) {
    LOGI("nativeInit called");
    
    // ------------------------------------------------------------------------
    // 步骤1: 检查是否已初始化
    // ------------------------------------------------------------------------
    // 如果用户快速切换Activity或旋转屏幕，可能会多次调用初始化
    // 这里先清理旧资源，再重新初始化
    if (g_initialized) {
        LOGI("Already initialized, cleaning up first");
        cleanupRenderer();
        terminateEGL();
        if (g_window) {
            ANativeWindow_release(g_window);
            g_window = nullptr;
        }
    }
    
    // ------------------------------------------------------------------------
    // 步骤2: 获取ANativeWindow
    // ------------------------------------------------------------------------
    // Java Surface -> Native ANativeWindow
    // ANativeWindow是Android原生窗口的抽象，可以直接用于EGL
    // 
    // 重要：每个SurfaceView都有独立的Surface
    // 所以即使创建多个OpenGL控件，这里获取的window也是不同的
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window) {
        LOGE("Failed to get ANativeWindow from surface");
        return JNI_FALSE;
    }
    
    // ------------------------------------------------------------------------
    // 步骤3: 初始化EGL
    // ------------------------------------------------------------------------
    // 创建OpenGL ES上下文并绑定到当前线程
    // 失败原因可能：
    // - 不支持OpenGL ES 3.0
    // - Surface已销毁
    // - 系统资源不足
    if (!initEGL(g_window)) {
        LOGE("Failed to initialize EGL");
        ANativeWindow_release(g_window);
        g_window = nullptr;
        return JNI_FALSE;
    }
    
    // ------------------------------------------------------------------------
    // 步骤4: 初始化渲染器
    // ------------------------------------------------------------------------
    // 编译shader、创建VAO/VBO等
    // 此时OpenGL上下文已经可用，所有OpenGL调用都是有效的
    if (!initRenderer()) {
        LOGE("Failed to initialize renderer");
        terminateEGL();
        ANativeWindow_release(g_window);
        g_window = nullptr;
        return JNI_FALSE;
    }
    
    // ------------------------------------------------------------------------
    // 步骤5: 标记初始化完成
    // ------------------------------------------------------------------------
    g_initialized = true;
    g_frameNumber = 0;  // 重置帧号
    
    LOGI("Initialization complete");
    return JNI_TRUE;
}

/**
 * @brief 渲染一帧
 * 
 * Kotlin调用示例：
 *   renderer.nativeRender()  // 在渲染线程的循环中调用
 * 
 * 时机：在渲染线程的while循环中持续调用（约60次/秒）
 * 
 * 工作流程：
 * 1. 创建渲染上下文（包含视口大小、投影矩阵、时间等）
 * 2. 调用渲染器的render方法（执行OpenGL绘制）
 * 3. 交换前后缓冲区（将渲染结果显示到屏幕）
 * 
 * @param env JNI环境指针
 * @param thiz Java对象引用
 * 
 * @note 此函数必须与nativeInit()在同一线程中调用
 *       这是OpenGL/EGL的硬性要求
 */
JNIEXPORT void JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeRender(JNIEnv* env, jobject thiz) {
    // ------------------------------------------------------------------------
    // 安全检查：确保已初始化
    // ------------------------------------------------------------------------
    if (!g_initialized || !g_renderer) {
        return;  // 静默返回，避免日志刷屏
    }
    
    // ------------------------------------------------------------------------
    // 步骤1: 创建渲染上下文
    // ------------------------------------------------------------------------
    // RenderContext是一个数据传输对象(DTO)，包含渲染所需的所有信息：
    // - 视口大小：告诉渲染器可用的绘制区域
    // - 投影矩阵：控制3D到2D的投影
    // - 帧号：用于动画（如旋转角度 = 帧号 * 速度）
    // - 时间差：用于与帧率无关的动画
    ViewportSize viewportSize(g_width, g_height);
    RenderContext context(viewportSize, g_projectionMatrix, 0.016f); // 0.016f ≈ 1/60秒
    context = context.withFrameNumber(g_frameNumber++);
    
    // ------------------------------------------------------------------------
    // 步骤2: 执行渲染
    // ------------------------------------------------------------------------
    // 这会调用TriangleRender::render()，执行：
    // 1. glClear() - 清屏
    // 2. 计算模型矩阵（旋转、平移等）
    // 3. shader.use() - 激活shader程序
    // 4. 设置uniform变量（MVP矩阵等）
    // 5. glDrawArrays() - 绘制三角形
    g_renderer->render(context);
    
    // ------------------------------------------------------------------------
    // 步骤3: 交换缓冲区
    // ------------------------------------------------------------------------
    // OpenGL使用双缓冲机制：
    // - 前缓冲(Front Buffer)：当前显示在屏幕上的图像
    // - 后缓冲(Back Buffer)：正在绘制的新图像
    // 
    // eglSwapBuffers()交换两个缓冲区，将新图像显示出来
    // 这避免了画面撕裂（Tearing）现象
    // 
    // 对于控件：每个EGLSurface都有独立的缓冲区
    // 所以多个OpenGL控件可以同时渲染，互不影响
    eglSwapBuffers(g_display, g_surface);
}

/**
 * @brief 处理控件尺寸变化
 * 
 * Kotlin调用示例：
 *   renderer.nativeResize(width, height)
 * 
 * 时机：当OpenGLSurfaceView的尺寸改变时调用
 * 触发条件：
 * - 初次显示（surfaceChanged）
 * - 屏幕旋转（横屏 ↔ 竖屏）
 * - 用户缩放控件（如果支持）
 * - 软键盘弹出/收起导致布局改变
 * 
 * @param env JNI环境指针
 * @param thiz Java对象引用
 * @param width 新的宽度（像素）
 * @param height 新的高度（像素）
 * 
 * @note 作为控件的优势：尺寸由XML布局决定
 *       例如 layout_height="300dp" 会被转换为具体像素数
 */
JNIEXPORT void JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeResize(JNIEnv* env, jobject thiz, jint width, jint height) {
    LOGI("nativeResize: %dx%d", width, height);
    
    // ------------------------------------------------------------------------
    // 步骤1: 保存新尺寸
    // ------------------------------------------------------------------------
    g_width = width;
    g_height = height;
    
    // ------------------------------------------------------------------------
    // 步骤2: 更新渲染器
    // ------------------------------------------------------------------------
    if (g_renderer && g_initialized) {
        // 更新OpenGL视口
        // glViewport(0, 0, width, height) 告诉OpenGL：
        // "你可以在这个矩形区域内绘制，坐标从(0,0)到(width,height)"
        g_renderer->resize(width, height);
        
        // 重新计算投影矩阵
        // 宽高比改变会影响图像的缩放比例
        // 例如：
        // - 方形控件(1:1): aspect = 1.0，图像不变形
        // - 宽屏控件(16:9): aspect = 1.78，图像水平拉伸
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        g_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
        
        // 作为控件使用的场景：
        // 假设XML中定义：
        // <OpenGLSurfaceView
        //     android:layout_width="match_parent"  -> width = 屏幕宽度
        //     android:layout_height="300dp"/>      -> height = 300dp转像素
        // 
        // 那么这里会自动适配这个尺寸，无需手动计算
    }
}

/**
 * @brief 清理所有OpenGL资源
 * 
 * Kotlin调用示例：
 *   renderer.nativeCleanup()  // 在渲染线程结束前调用
 * 
 * 时机：
 * - OpenGLSurfaceView被销毁（surfaceDestroyed）
 * - Activity进入后台
 * - 用户退出应用
 * 
 * 清理顺序：
 * 1. 清理渲染器（释放VAO/VBO/Shader等OpenGL对象）
 * 2. 终止EGL（销毁上下文和Surface）
 * 3. 释放ANativeWindow
 * 
 * @param env JNI环境指针
 * @param thiz Java对象引用
 * 
 * @note 关键：必须在创建资源的同一线程中调用
 *       如果在主线程清理，会导致OpenGL错误
 */
JNIEXPORT void JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeCleanup(JNIEnv* env, jobject thiz) {
    LOGI("nativeCleanup called");
    
    // ------------------------------------------------------------------------
    // 步骤1: 清理渲染器资源
    // ------------------------------------------------------------------------
    // 释放OpenGL对象：
    // - glDeleteBuffers() 删除VBO
    // - glDeleteVertexArrays() 删除VAO
    // - glDeleteProgram() 删除Shader程序
    cleanupRenderer();
    
    // ------------------------------------------------------------------------
    // 步骤2: 终止EGL
    // ------------------------------------------------------------------------
    // 释放OpenGL上下文和Surface
    // 这会释放大量GPU内存
    terminateEGL();
    
    // ------------------------------------------------------------------------
    // 步骤3: 释放ANativeWindow
    // ------------------------------------------------------------------------
    // 减少引用计数，允许系统回收Surface
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
    
    // ------------------------------------------------------------------------
    // 步骤4: 重置状态标志
    // ------------------------------------------------------------------------
    g_initialized = false;
    LOGI("Cleanup complete");
    
    // 作为控件的优势：
    // - OpenGLSurfaceView会在适当时机自动调用这个函数
    // - 不需要手动管理Activity生命周期
    // - 即使控件只是布局中的一小部分，清理也是完整的
}

/**
 * @brief 获取当前渲染器的名称
 * 
 * Kotlin调用示例：
 *   val name = renderer.nativeGetRendererName()
 *   Log.d("Renderer", "Using: $name")  // 输出: "Using: TriangleRender"
 * 
 * 用途：
 * - 调试信息
 * - UI显示（告诉用户当前使用的渲染器）
 * - 日志记录
 * 
 * @param env JNI环境指针
 * @param thiz Java对象引用
 * @return Java String对象，包含渲染器名称
 * 
 * @note 这是一个辅助函数，不影响渲染
 */
JNIEXPORT jstring JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeGetRendererName(JNIEnv* env, jobject thiz) {
    if (g_renderer) {
        // C++ string -> Java String
        // env->NewStringUTF() 创建一个新的Java字符串对象
        return env->NewStringUTF(g_renderer->getName().c_str());
    }
    return env->NewStringUTF("No Renderer");
}

} // extern "C"
