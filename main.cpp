#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#include "render_factory.hpp"
#include "render_config.hpp"
#include "render_context.hpp"

// 窗口大小
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// 全局变量
std::unique_ptr<IRenderer> g_renderer;
glm::mat4 g_projectionMatrix;

// 错误回调
void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// 窗口大小改变回调
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (g_renderer) {
        g_renderer->resize(width, height);
    }
    
    // 更新投影矩阵
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    g_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
}

// 键盘输入处理
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int main() {
    // 初始化GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 设置错误回调
    glfwSetErrorCallback(errorCallback);

    // 配置GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Triangle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 打印OpenGL版本信息
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);

    // 创建渲染器
    g_renderer = RenderFactory::create("triangle");
    if (!g_renderer) {
        std::cerr << "Failed to create renderer" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 设置错误回调
    g_renderer->setErrorCallback([](RenderError error, const std::string& msg) {
        std::cerr << "Render Error: " << msg << std::endl;
    });

    // 创建渲染配置
    RenderConfig config = RenderConfig::createTriangleConfig();

    // 初始化渲染器
    if (!g_renderer->initialize(config)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 初始化投影矩阵
    float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
    g_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);

    // 初始化视口
    g_renderer->resize(WINDOW_WIDTH, WINDOW_HEIGHT);

    // 渲染循环
    uint64_t frameNumber = 0;
    double lastTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        // 处理输入
        processInput(window);

        // 获取当前窗口大小
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // 创建渲染上下文
        ViewportSize viewportSize(width, height);
        RenderContext context(viewportSize, g_projectionMatrix, 0.016f);
        context = context.withFrameNumber(frameNumber++);

        // 渲染
        if (g_renderer) {
            g_renderer->render(context);
        }

        // 交换缓冲区和轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();

        // 计算FPS
        frameCount++;
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }

    // 清理
    if (g_renderer) {
        g_renderer->cleanup();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
