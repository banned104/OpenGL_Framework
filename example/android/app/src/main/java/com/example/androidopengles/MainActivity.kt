package com.example.androidopengles

import android.os.Bundle
import android.util.Log
import android.widget.FrameLayout
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat

/**
 * MainActivity - 应用程序的主入口Activity
 * 
 * Activity是Android应用的基本组件，代表一个用户可交互的屏幕。
 * 这个Activity负责：
 * 1. 创建并显示OpenGL渲染视图
 * 2. 管理Activity生命周期
 * 3. 处理全屏显示
 * 
 * Activity生命周期：
 * onCreate() -> onStart() -> onResume() -> [运行中] -> onPause() -> onStop() -> onDestroy()
 */
class MainActivity : ComponentActivity() {
    
    // 日志标签
    companion object {
        private const val TAG = "MainActivity"
    }
    
    // OpenGL渲染视图 - 使用懒加载，首次访问时才创建
    // lateinit表示这个变量会在之后初始化，不是在声明时
    private lateinit var glSurfaceView: OpenGLSurfaceView
    
    /**
     * Activity创建时调用
     * 
     * 这是Activity生命周期的第一个回调，在这里进行初始化工作：
     * 1. 调用父类的onCreate（必须）
     * 2. 设置全屏模式
     * 3. 创建并设置视图
     * 
     * @param savedInstanceState 保存的状态Bundle，用于恢复之前的状态（如屏幕旋转后）
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        Log.d(TAG, "onCreate: Activity正在创建")
        
        // 设置全屏沉浸式模式
        setupFullscreen()
        
        // 检查native库是否可用
        if (!NativeRenderer.isReady()) {
            // 如果库加载失败，显示错误信息
            showErrorView("无法加载native_renderer库\n请检查.so文件是否正确放置")
            return
        }
        
        // 创建主布局
        // FrameLayout是一个简单的布局容器，子视图会叠加显示
        val rootLayout = FrameLayout(this).apply {
            // apply块允许在创建对象后立即配置它
            // this指向FrameLayout实例
            
            // 设置背景色为黑色（在OpenGL加载前显示）
            setBackgroundColor(android.graphics.Color.BLACK)
        }
        
        // 创建OpenGL渲染视图
        glSurfaceView = OpenGLSurfaceView(this)
        
        // 将OpenGL视图添加到布局中
        // LayoutParams定义了视图如何放置在父布局中
        rootLayout.addView(
            glSurfaceView,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,  // 宽度填满父布局
                FrameLayout.LayoutParams.MATCH_PARENT   // 高度填满父布局
            )
        )
        
        // 可选：添加一个信息文本覆盖层
        val infoText = TextView(this).apply {
            text = "OpenGL ES 渲染中..."
            setTextColor(android.graphics.Color.WHITE)
            textSize = 14f
            // 设置内边距
            setPadding(32, 32, 32, 32)
        }
        rootLayout.addView(infoText)
        
        // 设置Activity的内容视图
        setContentView(rootLayout)
        
        Log.d(TAG, "onCreate: Activity创建完成")
    }
    
    /**
     * 设置全屏沉浸式模式
     * 
     * 隐藏状态栏和导航栏，让OpenGL内容占满整个屏幕
     */
    private fun setupFullscreen() {
        // WindowCompat提供了兼容性更好的窗口操作方法
        WindowCompat.setDecorFitsSystemWindows(window, false)
        
        // 获取窗口控制器
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        
        // 隐藏系统栏（状态栏和导航栏）
        controller.hide(WindowInsetsCompat.Type.systemBars())
        
        // 设置系统栏行为：滑动时临时显示，然后自动隐藏
        controller.systemBarsBehavior = 
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }
    
    /**
     * 显示错误视图
     * 
     * 当native库加载失败时，显示错误信息给用户
     * 
     * @param message 要显示的错误信息
     */
    private fun showErrorView(message: String) {
        val errorText = TextView(this).apply {
            text = message
            setTextColor(android.graphics.Color.RED)
            textSize = 18f
            gravity = android.view.Gravity.CENTER
            setBackgroundColor(android.graphics.Color.BLACK)
        }
        setContentView(errorText)
    }
    
    /**
     * Activity恢复到前台时调用
     * 
     * 当用户从其他应用切换回来，或者解锁屏幕后调用。
     * 可以在这里恢复渲染（如果需要的话）。
     */
    override fun onResume() {
        super.onResume()
        Log.d(TAG, "onResume: Activity恢复前台")
        // OpenGLSurfaceView会自动处理Surface的重建
    }
    
    /**
     * Activity进入后台时调用
     * 
     * 当用户按Home键或切换到其他应用时调用。
     * 可以在这里暂停渲染以节省电量。
     */
    override fun onPause() {
        super.onPause()
        Log.d(TAG, "onPause: Activity进入后台")
        // OpenGLSurfaceView会自动处理Surface的销毁
    }
    
    /**
     * Activity销毁时调用
     * 
     * 当用户按返回键退出应用，或系统回收内存时调用。
     * 所有资源应该在这里或之前释放。
     */
    override fun onDestroy() {
        Log.d(TAG, "onDestroy: Activity正在销毁")
        super.onDestroy()
    }
}