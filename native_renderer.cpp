/**
 * @file native_renderer.cpp
 * @brief Android JNI入口 - 提供给Java/Kotlin调用的Native渲染接口
 * 
 * 这个文件导出C接口，供Android应用通过JNI调用
 */

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <memory>
#include <string>

#include "render_factory.hpp"
#include "render_config.hpp"
#include "render_context.hpp"

#define LOG_TAG "NativeRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// 全局渲染器状态
// ============================================================================

namespace {
    // EGL相关
    EGLDisplay g_display = EGL_NO_DISPLAY;
    EGLSurface g_surface = EGL_NO_SURFACE;
    EGLContext g_context = EGL_NO_CONTEXT;
    ANativeWindow* g_window = nullptr;
    
    // 渲染器
    std::unique_ptr<IRenderer> g_renderer;
    RenderConfig g_config;
    glm::mat4 g_projectionMatrix(1.0f);
    
    // 视口大小
    int g_width = 0;
    int g_height = 0;
    uint64_t g_frameNumber = 0;
    
    bool g_initialized = false;
}

// ============================================================================
// EGL初始化和清理
// ============================================================================

static bool initEGL(ANativeWindow* window) {
    // 获取EGL显示
    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }
    
    // 初始化EGL
    EGLint majorVersion, minorVersion;
    if (!eglInitialize(g_display, &majorVersion, &minorVersion)) {
        LOGE("eglInitialize failed");
        return false;
    }
    LOGI("EGL version: %d.%d", majorVersion, minorVersion);
    
    // 配置属性
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_display, configAttribs, &config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("eglChooseConfig failed");
        return false;
    }
    
    // 创建Surface
    g_surface = eglCreateWindowSurface(g_display, config, window, nullptr);
    if (g_surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return false;
    }
    
    // 创建OpenGL ES 3.0上下文
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, contextAttribs);
    if (g_context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }
    
    // 绑定上下文
    if (!eglMakeCurrent(g_display, g_surface, g_surface, g_context)) {
        LOGE("eglMakeCurrent failed");
        return false;
    }
    
    // 获取Surface大小
    eglQuerySurface(g_display, g_surface, EGL_WIDTH, &g_width);
    eglQuerySurface(g_display, g_surface, EGL_HEIGHT, &g_height);
    LOGI("Surface size: %dx%d", g_width, g_height);
    
    // 打印OpenGL ES信息
    LOGI("GL_VENDOR: %s", glGetString(GL_VENDOR));
    LOGI("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LOGI("GL_VERSION: %s", glGetString(GL_VERSION));
    
    return true;
}

static void terminateEGL() {
    if (g_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (g_context != EGL_NO_CONTEXT) {
            eglDestroyContext(g_display, g_context);
            g_context = EGL_NO_CONTEXT;
        }
        
        if (g_surface != EGL_NO_SURFACE) {
            eglDestroySurface(g_display, g_surface);
            g_surface = EGL_NO_SURFACE;
        }
        
        eglTerminate(g_display);
        g_display = EGL_NO_DISPLAY;
    }
}

// ============================================================================
// 渲染器初始化和清理
// ============================================================================

static bool initRenderer() {
    // 创建渲染器
    g_renderer = RenderFactory::create("triangle");
    if (!g_renderer) {
        LOGE("Failed to create renderer");
        return false;
    }
    
    // 设置错误回调
    g_renderer->setErrorCallback([](RenderError error, const std::string& msg) {
        LOGE("Render Error [%d]: %s", static_cast<int>(error), msg.c_str());
    });
    
    // 创建配置
    g_config = RenderConfig::createTriangleConfig();
    
    // 初始化渲染器
    if (!g_renderer->initialize(g_config)) {
        LOGE("Failed to initialize renderer");
        return false;
    }
    
    // 设置视口和投影矩阵
    g_renderer->resize(g_width, g_height);
    
    float aspect = static_cast<float>(g_width) / static_cast<float>(g_height);
    g_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
    
    LOGI("Renderer initialized successfully");
    return true;
}

static void cleanupRenderer() {
    if (g_renderer) {
        g_renderer->cleanup();
        g_renderer.reset();
    }
}

// ============================================================================
// JNI导出函数
// ============================================================================

extern "C" {

/**
 * @brief 初始化渲染器
 * @param env JNI环境
 * @param thiz Java对象
 * @param surface Android Surface对象
 * @return 是否成功
 */
JNIEXPORT jboolean JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeInit(JNIEnv* env, jobject thiz, jobject surface) {
    LOGI("nativeInit called");
    
    if (g_initialized) {
        LOGI("Already initialized, cleaning up first");
        cleanupRenderer();
        terminateEGL();
        if (g_window) {
            ANativeWindow_release(g_window);
            g_window = nullptr;
        }
    }
    
    // 获取ANativeWindow
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window) {
        LOGE("Failed to get ANativeWindow from surface");
        return JNI_FALSE;
    }
    
    // 初始化EGL
    if (!initEGL(g_window)) {
        LOGE("Failed to initialize EGL");
        ANativeWindow_release(g_window);
        g_window = nullptr;
        return JNI_FALSE;
    }
    
    // 初始化渲染器
    if (!initRenderer()) {
        LOGE("Failed to initialize renderer");
        terminateEGL();
        ANativeWindow_release(g_window);
        g_window = nullptr;
        return JNI_FALSE;
    }
    
    g_initialized = true;
    g_frameNumber = 0;
    
    LOGI("Initialization complete");
    return JNI_TRUE;
}

/**
 * @brief 渲染一帧
 * @param env JNI环境
 * @param thiz Java对象
 */
JNIEXPORT void JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeRender(JNIEnv* env, jobject thiz) {
    if (!g_initialized || !g_renderer) {
        return;
    }
    
    // 创建渲染上下文
    ViewportSize viewportSize(g_width, g_height);
    RenderContext context(viewportSize, g_projectionMatrix, 0.016f);
    context = context.withFrameNumber(g_frameNumber++);
    
    // 渲染
    g_renderer->render(context);
    
    // 交换缓冲区
    eglSwapBuffers(g_display, g_surface);
}

/**
 * @brief 处理Surface大小变化
 * @param env JNI环境
 * @param thiz Java对象
 * @param width 新宽度
 * @param height 新高度
 */
JNIEXPORT void JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeResize(JNIEnv* env, jobject thiz, jint width, jint height) {
    LOGI("nativeResize: %dx%d", width, height);
    
    g_width = width;
    g_height = height;
    
    if (g_renderer && g_initialized) {
        g_renderer->resize(width, height);
        
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        g_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
    }
}

/**
 * @brief 清理资源
 * @param env JNI环境
 * @param thiz Java对象
 */
JNIEXPORT void JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeCleanup(JNIEnv* env, jobject thiz) {
    LOGI("nativeCleanup called");
    
    cleanupRenderer();
    terminateEGL();
    
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
    
    g_initialized = false;
    LOGI("Cleanup complete");
}

/**
 * @brief 获取渲染器名称
 * @param env JNI环境
 * @param thiz Java对象
 * @return 渲染器名称
 */
JNIEXPORT jstring JNICALL
Java_com_example_androidopengles_NativeRenderer_nativeGetRendererName(JNIEnv* env, jobject thiz) {
    if (g_renderer) {
        return env->NewStringUTF(g_renderer->getName().c_str());
    }
    return env->NewStringUTF("No Renderer");
}

} // extern "C"
