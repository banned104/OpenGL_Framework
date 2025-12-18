// ============================================================================
// Kotlin基础语法说明（给不懂Kotlin的开发者）
// ============================================================================
// 
// package: 定义包名，相当于C++的namespace
// import: 导入其他类，相当于C++的#include
// class: 定义类
// val: 不可变变量（相当于C++的const，Java的final）
// var: 可变变量（相当于普通变量）
// ?: 可空类型标记（表示变量可以是null）
// @注解: 提供额外信息（如@Volatile表示多线程可见）
// 
// ============================================================================

package com.example.androidopengles

// Android框架类
import android.content.Context       // Android上下文：包含应用环境信息
import android.util.AttributeSet     // XML属性集：从布局文件读取属性
import android.util.Log              // 日志工具：输出到Logcat
import android.view.Surface          // 绘图表面：OpenGL渲染的目标
import android.view.SurfaceHolder    // Surface管理器：监听Surface生命周期
import android.view.SurfaceView      // Surface视图：提供独立绘图表面的View

/**
 * OpenGLSurfaceView - 自定义的OpenGL渲染控件
 * 
 * ============================================================================
 * 类继承关系：
 * ============================================================================
 * OpenGLSurfaceView
 *   ↓ extends (继承)
 * SurfaceView (Android提供的特殊View，有独立的绘图表面)
 *   ↓ extends
 * View (所有Android UI控件的基类)
 * 
 * ============================================================================
 * 接口实现：
 * ============================================================================
 * SurfaceHolder.Callback (监听Surface的创建/改变/销毁)
 * 
 * ============================================================================
 * 为什么使用SurfaceView而不是普通View？
 * ============================================================================
 * 1. 普通View在UI线程绘制，会阻塞界面
 * 2. SurfaceView有独立的绘图表面，可以在后台线程绘制
 * 3. 适合高频率更新的内容（游戏、视频、OpenGL）
 * 4. 不会阻塞UI线程，不会卡顿
 * 
 * ============================================================================
 * OpenGL/EGL线程模型（关键知识点！）
 * ============================================================================
 * 问题：为什么黑屏？
 * 答案：EGL上下文是线程绑定的！
 * 
 * 错误做法：
 *   主线程: surfaceCreated() -> nativeInit() -> 创建EGL上下文
 *   渲染线程: nativeRender() -> 使用EGL -> 失败！（上下文不在这个线程）
 * 
 * 正确做法（本代码的实现）：
 *   主线程: surfaceCreated() -> 只保存Surface引用
 *   渲染线程: nativeInit() -> 创建EGL上下文
 *   渲染线程: nativeRender() -> 使用EGL -> 成功！（同一线程）
 * 
 * ============================================================================
 * 
 * @param context Android上下文，通常是Activity实例
 *                提供访问系统资源的能力（如窗口管理器、资源文件等）
 * 
 * @param attrs XML属性集（可选）
 *              当在XML布局中使用此控件时，会传递XML中定义的属性
 *              例如：android:layout_width="300dp" 这些属性
 */
class OpenGLSurfaceView @JvmOverloads constructor(
    // -----------------------------------------------------------------------
    // 构造函数参数
    // -----------------------------------------------------------------------
    // Kotlin特性：主构造函数直接写在类名后面
    // @JvmOverloads: 生成多个Java重载构造函数
    //   - OpenGLSurfaceView(Context)
    //   - OpenGLSurfaceView(Context, AttributeSet)
    // 这样Java代码和Kotlin代码都可以方便地创建实例
    
    context: Context,                // 必需参数：Android上下文
    attrs: AttributeSet? = null      // 可选参数：XML属性（？表示可以为null）
    
// : SurfaceView(...) 表示继承SurfaceView，并调用父类构造函数
// , SurfaceHolder.Callback 表示实现此接口
) : SurfaceView(context, attrs), SurfaceHolder.Callback {
    
    // ========================================================================
    // companion object - Kotlin的静态成员区域
    // ========================================================================
    // 相当于Java的static块
    // 所有实例共享这些成员
    companion object {
        // const val 定义编译时常量（相当于Java的 public static final）
        private const val TAG = "OpenGLSurfaceView"  // 日志标签，用于过滤Logcat
    }
    
    // ========================================================================
    // 成员变量（实例变量）
    // ========================================================================
    
    // ------------------------------------------------------------------------
    // Native层接口
    // ------------------------------------------------------------------------
    // val = 不可变引用（引用本身不能改变，但对象内容可以改变）
    // = NativeRenderer() 创建实例（Kotlin不需要写new关键字）
    private val renderer = NativeRenderer()  // JNI桥接对象，负责调用C++代码
    
    // ------------------------------------------------------------------------
    // 渲染线程管理
    // ------------------------------------------------------------------------
    // var = 可变变量（可以重新赋值）
    // ? 表示可空类型（可以是null）
    // RenderThread? 相当于Java的 RenderThread（可空）
    private var renderThread: RenderThread? = null  // 渲染线程引用
    
    // ------------------------------------------------------------------------
    // 多线程共享变量
    // ------------------------------------------------------------------------
    // @Volatile 注解：保证多线程可见性
    // 相当于C++的 std::atomic 或 Java的 volatile
    // 
    // 为什么需要？
    // - isRendering 在主线程写入，渲染线程读取
    // - 没有@Volatile，渲染线程可能读到旧值（CPU缓存问题）
    // - 有了@Volatile，保证所有线程看到的值是一致的
    
    @Volatile
    private var isRendering = false  // Boolean类型：true表示继续渲染，false表示停止
    
    @Volatile
    private var currentSurface: Surface? = null  // Surface引用，传递给渲染线程
    
    @Volatile
    private var surfaceWidth = 0     // Int类型：Surface宽度（像素）
    @Volatile
    private var surfaceHeight = 0    // Int类型：Surface高度（像素）
    
    @Volatile
    private var needsInit = false    // 标志位：是否需要初始化EGL
    
    @Volatile
    private var needsResize = false  // 标志位：是否需要更新视口尺寸
    
    // ========================================================================
    // init 块 - 初始化代码
    // ========================================================================
    // 相当于Java构造函数中的代码
    // 在对象创建后立即执行
    init {
        // holder 是从父类SurfaceView继承的属性
        // 类型是 SurfaceHolder，用于管理Surface的生命周期
        // 
        // addCallback(this) 注册回调
        // - this 指向当前OpenGLSurfaceView实例
        // - 因为实现了SurfaceHolder.Callback接口
        // - 所以当Surface创建/改变/销毁时，会调用我们的方法：
        //   * surfaceCreated()
        //   * surfaceChanged()
        //   * surfaceDestroyed()
        holder.addCallback(this)
        
        // Log.d() 输出调试日志
        // 参数1: TAG（日志标签）
        // 参数2: 日志消息
        // 在Android Studio的Logcat中可以通过TAG过滤
        Log.d(TAG, "OpenGLSurfaceView已创建")
    }
    
    // ========================================================================
    // SurfaceHolder.Callback 接口实现
    // ========================================================================
    // 这三个方法会在主线程（UI线程）被系统自动调用
    // override 关键字：表示重写父类/接口的方法（相当于C++的override）
    
    /**
     * Surface创建完成时调用
     * 
     * -----------------------------------------------------------------------
     * 调用时机（系统自动调用）：
     * -----------------------------------------------------------------------
     * 1. OpenGLSurfaceView首次显示在屏幕上
     * 2. 从后台返回前台（之前被销毁的Surface重新创建）
     * 3. 从其他Activity返回时
     * 
     * -----------------------------------------------------------------------
     * 执行线程：主线程（UI线程）
     * -----------------------------------------------------------------------
     * 
     * -----------------------------------------------------------------------
     * 关键设计决策：为什么不在这里初始化EGL？
     * -----------------------------------------------------------------------
     * 错误做法：
     *   surfaceCreated(主线程) {
     *       nativeInit(surface)  // EGL上下文绑定到主线程
     *   }
     *   渲染线程.run() {
     *       nativeRender()  // 失败！EGL上下文不在这个线程
     *   }
     * 
     * 正确做法（本代码）：
     *   surfaceCreated(主线程) {
     *       保存Surface引用
     *       启动渲染线程
     *   }
     *   渲染线程.run() {
     *       nativeInit(surface)  // EGL上下文绑定到渲染线程
     *       nativeRender()       // 成功！同一线程
     *   }
     * 
     * @param holder SurfaceHolder对象，管理Surface
     */
    override fun surfaceCreated(holder: SurfaceHolder) {
        // holder.surface 获取实际的Surface对象
        
        Log.d(TAG, "Surface已创建（主线程）")
        
        // ====================================================================
        // 步骤1: 检查Native库是否加载成功
        // ====================================================================
        // ! 是Kotlin的非空断言运算符
        // !NativeRenderer.isReady() 相当于 NativeRenderer.isReady() == false
        if (!NativeRenderer.isReady()) {
            // Log.e() 输出错误日志（红色）
            Log.e(TAG, "Native库未加载")
            return  // 提前返回，不继续执行
        }
        
        // ====================================================================
        // 步骤2: 保存Surface引用
        // ====================================================================
        // 不在主线程初始化EGL，只保存Surface
        // holder.surface 获取Surface对象
        currentSurface = holder.surface
        needsInit = true  // 设置标志：需要初始化
        
        // ====================================================================
        // 步骤3: 启动渲染线程
        // ====================================================================
        isRendering = true  // 设置渲染标志为true
        
        // RenderThread() 创建渲染线程实例
        // Kotlin特性：不需要写new关键字
        renderThread = RenderThread()
        
        // ?. 是安全调用运算符（Safe Call Operator）
        // 相当于：if (renderThread != null) renderThread.start()
        // 如果renderThread是null，不会执行start()，也不会抛出NullPointerException
        renderThread?.start()  // 启动线程，开始执行run()方法
        
        // 注意：start()返回后，run()方法在新线程中异步执行
        // 主线程继续执行，不会等待渲染线程
    }
    
    /**
     * Surface尺寸改变时调用
     * 
     * -----------------------------------------------------------------------
     * 调用时机（系统自动调用）：
     * -----------------------------------------------------------------------
     * 1. Surface创建后立即调用（在surfaceCreated之后）
     * 2. 屏幕旋转（横屏 ↔ 竖屏）
     * 3. 软键盘弹出/收起导致布局改变
     * 4. 窗口大小改变（多窗口模式）
     * 
     * -----------------------------------------------------------------------
     * 执行线程：主线程（UI线程）
     * -----------------------------------------------------------------------
     * 
     * @param holder SurfaceHolder对象
     * @param format Surface的像素格式（如RGBA_8888）
     * @param width 新的宽度（像素）
     * @param height 新的高度（像素）
     */
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // ====================================================================
        // 字符串模板（String Template）
        // ====================================================================
        // ${变量名} 在字符串中插入变量的值
        // 相当于Java的 String.format("...", width, height)
        // 例如：width=1080, height=1920
        //       输出："Surface尺寸改变: 1080x1920（主线程）"
        Log.d(TAG, "Surface尺寸改变: ${width}x${height}（主线程）")
        
        // ====================================================================
        // 保存新尺寸
        // ====================================================================
        // 不直接调用nativeResize()，因为：
        // 1. EGL上下文不在主线程
        // 2. 让渲染线程自己处理，避免线程同步问题
        surfaceWidth = width
        surfaceHeight = height
        needsResize = true  // 设置标志，渲染线程会检测到并处理
        
        // 作为控件使用的例子：
        // XML中定义：
        //   <OpenGLSurfaceView
        //       android:layout_width="match_parent"  -> width可能是1080px
        //       android:layout_height="300dp"/>      -> height可能是900px
        // 这里会自动收到这些尺寸，无需手动计算
    }
    
    /**
     * Surface销毁时调用
     * 
     * -----------------------------------------------------------------------
     * 调用时机（系统自动调用）：
     * -----------------------------------------------------------------------
     * 1. Activity进入后台（按Home键）
     * 2. 屏幕锁定
     * 3. Activity被销毁
     * 4. OpenGLSurfaceView从布局中移除
     * 
     * -----------------------------------------------------------------------
     * 执行线程：主线程（UI线程）
     * -----------------------------------------------------------------------
     * 
     * -----------------------------------------------------------------------
     * 关键任务：必须停止渲染线程并等待其结束
     * -----------------------------------------------------------------------
     * 如果不等待线程结束就返回，会导致：
     * 1. 渲染线程继续访问已销毁的Surface -> 崩溃
     * 2. EGL资源没有正确释放 -> 内存泄漏
     * 3. OpenGL上下文仍然占用GPU资源
     * 
     * @param holder SurfaceHolder对象
     */
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.d(TAG, "Surface已销毁（主线程）")
        
        // ====================================================================
        // 步骤1: 停止渲染循环
        // ====================================================================
        // 设置标志为false，渲染线程的while循环会检测到并退出
        isRendering = false
        
        // ====================================================================
        // 步骤2: 等待渲染线程结束
        // ====================================================================
        // ?. 安全调用：如果renderThread是null，不执行后面的代码
        // let { } 是Kotlin的作用域函数
        // 
        // 解释 let 的工作原理：
        // renderThread?.let { thread ->
        //     // 这里的thread就是renderThread（非null的情况）
        //     // 可以安全地使用thread，不用担心null
        // }
        // 
        // 相当于Java的：
        // if (renderThread != null) {
        //     Thread thread = renderThread;
        //     // 使用thread
        // }
        renderThread?.let { thread ->
            // try-catch 异常处理（相当于C++的try-catch）
            try {
                // thread.join(2000)
                // 阻塞当前线程（主线程），等待渲染线程结束
                // 参数：最多等待2000毫秒（2秒）
                // 
                // 如果渲染线程在2秒内结束，立即返回
                // 如果2秒后还没结束，也返回（超时）
                thread.join(2000)  // 单位：毫秒
                
                Log.d(TAG, "渲染线程已停止")
                
            } catch (e: InterruptedException) {
                // 捕获InterruptedException异常
                // e是异常对象，类型是InterruptedException
                // 
                // 这个异常表示：当前线程在等待时被其他线程中断
                // （类似于C++的线程中断）
                Log.e(TAG, "等待渲染线程时被中断")
            }
        }
        
        // ====================================================================
        // 步骤3: 清理引用
        // ====================================================================
        // 设置为null，帮助垃圾回收器回收内存
        // Kotlin的null安全特性：
        // - 如果变量声明为 Type?，可以赋值null
        // - 如果变量声明为 Type（无?），不能赋值null（编译错误）
        renderThread = null
        currentSurface = null
    }
    
    // ========================================================================
    // 公共方法（可被外部调用）
    // ========================================================================
    
    /**
     * 获取渲染器名称
     * 
     * 用途：调试信息或UI显示
     * 
     * Kotlin特性：
     * - fun 关键字定义函数
     * - : String 表示返回类型是String
     * - 没有访问修饰符默认是public
     * 
     * @return 渲染器名称，如"TriangleRender"
     */
    fun getRendererName(): String {
        // ====================================================================
        // try-catch 表达式
        // ====================================================================
        // Kotlin特性：try可以作为表达式，有返回值
        // 
        // 相当于Java的：
        // String result;
        // try {
        //     result = renderer.nativeGetRendererName();
        // } catch (Exception e) {
        //     result = "未初始化";
        // }
        // return result;
        
        return try {
            // 正常情况：调用JNI方法获取名称
            renderer.nativeGetRendererName()
            
        } catch (e: Exception) {
            // 异常情况：如果Native库未加载或渲染器未初始化
            // e: Exception 表示捕获所有Exception类型的异常
            "未初始化"  // Kotlin的最后一个表达式的值作为返回值
        }
    }
    
    // ========================================================================
    // 内部类：RenderThread - 渲染线程
    // ========================================================================
    // 
    // inner class 关键字：内部类
    // - 可以访问外部类的成员变量和方法
    // - 相当于Java的非静态内部类
    // 
    // : Thread("...") 表示继承Thread类
    // - 参数"OpenGL-RenderThread"是线程名称
    // - 在调试时可以看到这个名称
    // 
    // ========================================================================
    // 为什么需要单独的渲染线程？
    // ========================================================================
    // 1. OpenGL渲染不能在主线程（会阻塞UI，导致卡顿）
    // 2. EGL上下文必须在创建它的线程中使用
    // 3. 渲染循环需要持续运行（60次/秒）
    // 
    // ========================================================================
    // 线程工作流程：
    // ========================================================================
    // 1. surfaceCreated() 在主线程启动此线程
    // 2. run() 方法在新线程中执行
    // 3. 循环检查标志，执行初始化/渲染/清理
    // 4. surfaceDestroyed() 在主线程等待此线程结束
    // 
    /**
     * RenderThread - OpenGL渲染线程
     * 
     * 关键：EGL上下文必须在这个线程中创建和使用！
     * - nativeInit() 在这里调用 -> EGL上下文绑定到这个线程
     * - nativeRender() 在这里调用 -> 使用同一个线程的上下文
     * - nativeCleanup() 在这里调用 -> 在同一个线程清理
     */
    inner class RenderThread : Thread("OpenGL-RenderThread") {
        
        // ====================================================================
        // 线程局部变量
        // ====================================================================
        // private 表示只在这个类内部可见
        // var 表示可变（可以从false改为true）
        private var initialized = false  // 标记EGL是否已初始化
        
        // ====================================================================
        // override fun run() - 线程执行方法
        // ====================================================================
        // 当调用 thread.start() 时，系统会创建新线程并执行这个方法
        // 
        // 注意：
        // - 不要直接调用 run()，要调用 start()
        // - start() 会创建新线程，run() 只是普通方法调用
        override fun run() {
            Log.d(TAG, "渲染线程已启动")
            
            // ================================================================
            // 渲染主循环
            // ================================================================
            // while (条件) { 循环体 }
            // 当isRendering为true时，持续执行
            // 当surfaceDestroyed()设置isRendering=false时，循环退出
            while (isRendering) {
                
                // ============================================================
                // 步骤1: 检查是否需要初始化EGL
                // ============================================================
                // && 是逻辑与运算符（相当于C++的&&）
                // !initialized 相当于 initialized == false
                // 
                // 只有当：
                // 1. needsInit为true（主线程设置的标志）
                // 2. 且 initialized为false（还未初始化）
                // 才执行初始化
                if (needsInit && !initialized) {
                    // val 声明不可变变量（相当于const）
                    // 从volatile变量读取Surface引用
                    val surface = currentSurface
                    
                    // 检查Surface是否有效
                    // != null 检查是否非空
                    // && surface.isValid 检查Surface是否有效（未被销毁）
                    if (surface != null && surface.isValid) {
                        Log.d(TAG, "在渲染线程中初始化EGL...")
                        
                        // 调用JNI方法初始化EGL
                        // if 也可以作为表达式（有返回值）
                        if (renderer.nativeInit(surface)) {
                            // 初始化成功
                            initialized = true
                            needsInit = false  // 清除标志
                            Log.d(TAG, "EGL初始化成功（渲染线程）")
                        } else {
                            // 初始化失败
                            Log.e(TAG, "EGL初始化失败")
                            break  // 跳出while循环，线程结束
                        }
                    }
                }
                
                // ============================================================
                // 步骤2: 检查是否需要更新视口尺寸
                // ============================================================
                if (needsResize && initialized) {
                    // 调用JNI方法更新视口
                    renderer.nativeResize(surfaceWidth, surfaceHeight)
                    needsResize = false  // 清除标志
                    
                    // 字符串模板：${变量}在字符串中插入变量值
                    Log.d(TAG, "视口已更新: ${surfaceWidth}x${surfaceHeight}（渲染线程）")
                }
                
                // ============================================================
                // 步骤3: 渲染一帧
                // ============================================================
                // 只有初始化完成后才渲染
                if (initialized) {
                    // 调用JNI方法渲染
                    // 这会执行：
                    // 1. glClear() 清屏
                    // 2. glDrawArrays() 绘制三角形
                    // 3. eglSwapBuffers() 交换缓冲区
                    renderer.nativeRender()
                }
                
                // ============================================================
                // 步骤4: 控制帧率
                // ============================================================
                // 目标：60 FPS (Frame Per Second，每秒60帧)
                // 计算：1000ms / 60 ≈ 16.67ms
                // 所以每帧之间休眠16ms
                try {
                    // sleep(毫秒) 让当前线程休眠
                    // 相当于C++的 std::this_thread::sleep_for()
                    sleep(16)  // 16毫秒 ≈ 60 FPS
                    
                } catch (e: InterruptedException) {
                    // 如果线程在休眠时被中断，退出循环
                    // 这是一种安全的线程终止机制
                    break
                }
            }
            
            // ================================================================
            // 循环结束后：清理资源
            // ================================================================
            // 注意：必须在同一个线程中清理EGL资源
            // 如果在主线程清理，会导致OpenGL错误
            if (initialized) {
                Log.d(TAG, "在渲染线程中清理EGL...")
                
                // 调用JNI方法清理
                // 这会执行：
                // 1. glDeleteBuffers() 删除VBO
                // 2. glDeleteProgram() 删除Shader
                // 3. eglDestroyContext() 销毁上下文
                renderer.nativeCleanup()
                
                initialized = false  // 重置标志
            }
            
            Log.d(TAG, "渲染线程已结束")
        }
    }
}

// ============================================================================
// Kotlin语法总结（给不懂Kotlin的开发者）
// ============================================================================
// 
// 1. 变量声明：
//    val name: Type = value  // 不可变（相当于const）
//    var name: Type = value  // 可变
// 
// 2. 可空类型：
//    Type?  // 可以为null
//    Type   // 不能为null
// 
// 3. 安全调用：
//    object?.method()  // 如果object是null，不执行method()
// 
// 4. 类型推断：
//    val x = 5  // 编译器自动推断类型为Int
// 
// 5. 字符串模板：
//    "值是 ${variable}"  // 在字符串中插入变量
// 
// 6. 函数定义：
//    fun functionName(param: Type): ReturnType { }
// 
// 7. 类定义：
//    class ClassName : ParentClass(), Interface
// 
// 8. 构造函数：
//    class Name(param: Type)  // 主构造函数在类名后
// 
// 9. 作用域函数：
//    object?.let { it -> ... }  // 如果非null，执行代码块
// 
// 10. try表达式：
//     val result = try { ... } catch (e: Exception) { ... }
// 
// ============================================================================
