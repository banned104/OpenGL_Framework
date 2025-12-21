#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "render_factory.hpp"
#include "render_config.hpp"
#include "render_context.hpp"

#include "Component/ThreadPool/multi-thread.hpp"

/**
 * @brief Application类 - 封装整个OpenGL应用程序的生命周期
 * 
 * 单一职责: 管理窗口、渲染循环和资源
 */
class Application {
public:
    Application(int width, int height, const std::string& title)
        : m_width(width)
        , m_height(height)
        , m_title(title)
        , m_window(nullptr)
        , m_frameNumber(0)
        , m_frameCount(0)
        , m_lastTime(0.0)
    {
    }

    ~Application() {
        shutdown();
    }

    // 禁止拷贝
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief 初始化应用程序
     */
    bool initialize() {
        // 初始化GLFW
        if (!initializeGLFW()) {
            return false;
        }

        // 初始化GLAD
        if (!initializeGLAD()) {
            return false;
        }

        // 打印OpenGL信息
        printGLInfo();

        // 初始化OpenGL状态
        initializeGLState();

        // 初始化渲染器
        if (!initializeRenderer()) {
            return false;
        }

        // 初始化投影矩阵
        updateProjectionMatrix();

        return true;
    }

    /**
     * @brief 运行主循环
     */
    void run() {
        m_lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(m_window)) {
            // 处理输入
            processInput();

            // 更新
            update();

            // 渲染
            render();

            // 交换缓冲区
            glfwSwapBuffers(m_window);
            glfwPollEvents();

            // 更新FPS
            updateFPS();
        }
    }

    /**
     * @brief 关闭应用程序
     */
    void shutdown() {
        if (m_renderer) {
            m_renderer->cleanup();
            m_renderer.reset();
        }

        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }

        glfwTerminate();
    }

private:
    // ============ 初始化方法 ============

    bool initializeGLFW() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        glfwSetErrorCallback([](int error, const char* description) {
            std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
        });

        // 配置OpenGL版本
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        // 创建窗口
        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!m_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_window);

        // 设置用户指针，用于回调函数访问Application实例
        glfwSetWindowUserPointer(m_window, this);

        // 设置回调
        glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
        glfwSetKeyCallback(m_window, keyCallback);

        return true;
    }

    bool initializeGLAD() {
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return false;
        }
        return true;
    }

    void printGLInfo() {
        std::cout << "========================================" << std::endl;
        std::cout << "OpenGL Vendor:   " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "========================================" << std::endl;
    }

    void initializeGLState() {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    // 对 m_renderer 进行创建和配置
    bool initializeRenderer() {
        // 使用工厂创建渲染器
        m_renderer = RenderFactory::create("triangle");
        if (!m_renderer) {
            std::cerr << "Failed to create renderer" << std::endl;
            return false;
        }

        // 设置错误回调
        m_renderer->setErrorCallback([](RenderError error, const std::string& msg) {
            std::cerr << "Render Error [" << static_cast<int>(error) << "]: " << msg << std::endl;
        });

        // 创建配置并初始化
        m_config = RenderConfig::createTriangleConfig();
        if (!m_renderer->initialize(m_config)) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }

        // 初始化视口
        m_renderer->resize(m_width, m_height);

        return true;
    }

    // ============ 回调处理 ============

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
        auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->onResize(width, height);
        }
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        if (app) {
            app->onKeyPress(key, scancode, action, mods);
        }
    }

    void onResize(int width, int height) {
        m_width = width;
        m_height = height;

        if (m_renderer) {
            m_renderer->resize(width, height);
        }

        updateProjectionMatrix();
    }

    void onKeyPress(int key, int scancode, int action, int mods) {
        (void)scancode;
        (void)mods;

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(m_window, true);
        }
    }

    // ============ 主循环方法 ============

    void processInput() {
        // 额外的输入处理可以在这里添加
    }

    void update() {
        // 更新逻辑可以在这里添加
    }

    void render() {
        if (!m_renderer) return;

        // 创建渲染上下文
        ViewportSize viewportSize(m_width, m_height);
        RenderContext context(viewportSize, m_projectionMatrix, 0.016f);
        context = context.withFrameNumber(m_frameNumber++);

        // 执行渲染
        m_renderer->render(context);
    }

    void updateProjectionMatrix() {
        if (m_width <= 0 || m_height <= 0) return;

        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        m_projectionMatrix = glm::perspective(glm::radians(30.0f), aspect, 3.0f, 10.0f);
    }

    void updateFPS() {
        m_frameCount++;
        double currentTime = glfwGetTime();

        if (currentTime - m_lastTime >= 1.0) {
            std::cout << "FPS: " << m_frameCount << std::endl;
            m_frameCount = 0;
            m_lastTime = currentTime;
        }
    }

private:
    // 窗口属性
    int m_width;
    int m_height;
    std::string m_title;
    GLFWwindow* m_window;

    // 渲染相关
    std::unique_ptr<IRenderer> m_renderer;
    RenderConfig m_config;
    glm::mat4 m_projectionMatrix;

    // 帧计数
    uint64_t m_frameNumber;
    int m_frameCount;
    double m_lastTime;
};

// ============ 主函数 ============
int main() {
    // ----- ThreadPool -----
    // 线程创建启动
    std::thread t1(ThreadTestClass::ThreadInstance1);
    // 获取当前线程ID
    std::cout << "Main thread id: " << std::this_thread::get_id() << std::endl;

    {
        // 括号内创建线程 运行到外部之后 t2_InBrace 会被销毁(子线程对象销毁) 但是子线程还是在运行 此时报错
        // error: abort() has been called
        // std::thread t2_InBrace(ThreadTestClass::ThreadInstance1);

        // 可以做一个分离 .detach 变为后台运行的守护线程, 此时子线程与主线程无关
        std::thread t3_DetechTest(ThreadTestClass::ThreadInstance1);
        t3_DetechTest.detach();
        // 坑: 主线程退出之后子线程不一定退出,继续运行, 主线程退出时内存空间被释放,子线程访问这些空间(静态对象全局变量等)会导致程序崩溃
        // 所以 detach的子线程不要访问任何外部变量. 而且一般不用detach.
    }
    // ----- ThreadPool -----
    
    Application app(800, 600, "OpenGL Triangle");

    if (!app.initialize()) {
        std::cerr << "Application initialization failed!" << std::endl;
        return -1;
    }

    app.run();

    // ----- ThreadPool -----
    t1.join();
    // ----- ThreadPool -----
    return 0;
}
