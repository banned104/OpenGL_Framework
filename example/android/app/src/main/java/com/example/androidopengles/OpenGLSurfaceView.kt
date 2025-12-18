package com.example.androidopengles

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

/**
 * OpenGLSurfaceView - 自定义的OpenGL渲染视图
 * 
 * 这个类继承自SurfaceView，提供一个可以进行OpenGL渲染的画布。
 * 
 * 重要：OpenGL/EGL的上下文是线程绑定的！
 * - EGL上下文只能在创建它的线程中使用
 * - 所以我们必须在渲染线程中初始化EGL，而不是在主线程
 * 
 * @param context Android上下文，通常是Activity
 * @param attrs XML属性集（如果在XML布局中使用）
 */
class OpenGLSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : SurfaceView(context, attrs), SurfaceHolder.Callback {
    
    companion object {
        private const val TAG = "OpenGLSurfaceView"
    }
    
    // Native渲染器实例
    private val renderer = NativeRenderer()
    
    // 渲染线程
    private var renderThread: RenderThread? = null
    
    // 标记渲染是否应该继续
    @Volatile
    private var isRendering = false
    
    // 保存Surface引用，传递给渲染线程
    @Volatile
    private var currentSurface: Surface? = null
    
    // 保存尺寸，传递给渲染线程
    @Volatile
    private var surfaceWidth = 0
    @Volatile
    private var surfaceHeight = 0
    
    // 标记是否需要重新初始化
    @Volatile
    private var needsInit = false
    
    // 标记是否需要更新尺寸
    @Volatile
    private var needsResize = false
    
    init {
        holder.addCallback(this)
        Log.d(TAG, "OpenGLSurfaceView已创建")
    }
    
    /**
     * Surface创建完成时调用（主线程）
     * 
     * 注意：不要在这里初始化EGL！
     * 我们只保存Surface引用，让渲染线程去初始化。
     */
    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.d(TAG, "Surface已创建（主线程）")
        
        if (!NativeRenderer.isReady()) {
            Log.e(TAG, "Native库未加载")
            return
        }
        
        // 保存Surface，稍后在渲染线程中使用
        currentSurface = holder.surface
        needsInit = true
        
        // 启动渲染线程（EGL初始化将在线程内部进行）
        isRendering = true
        renderThread = RenderThread()
        renderThread?.start()
    }
    
    /**
     * Surface尺寸改变时调用（主线程）
     */
    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.d(TAG, "Surface尺寸改变: ${width}x${height}（主线程）")
        
        // 保存尺寸，让渲染线程处理
        surfaceWidth = width
        surfaceHeight = height
        needsResize = true
    }
    
    /**
     * Surface销毁时调用（主线程）
     */
    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.d(TAG, "Surface已销毁（主线程）")
        
        // 停止渲染循环
        isRendering = false
        
        // 等待渲染线程结束
        renderThread?.let { thread ->
            try {
                thread.join(2000)  // 最多等待2秒
                Log.d(TAG, "渲染线程已停止")
            } catch (e: InterruptedException) {
                Log.e(TAG, "等待渲染线程时被中断")
            }
        }
        renderThread = null
        currentSurface = null
    }
    
    /**
     * 获取渲染器名称
     */
    fun getRendererName(): String {
        return try {
            renderer.nativeGetRendererName()
        } catch (e: Exception) {
            "未初始化"
        }
    }
    
    /**
     * RenderThread - 渲染线程
     * 
     * 关键：EGL上下文必须在这个线程中创建和使用！
     * - nativeInit() 在这里调用 -> EGL上下文绑定到这个线程
     * - nativeRender() 在这里调用 -> 使用同一个线程的上下文
     * - nativeCleanup() 在这里调用 -> 在同一个线程清理
     */
    inner class RenderThread : Thread("OpenGL-RenderThread") {
        
        private var initialized = false
        
        override fun run() {
            Log.d(TAG, "渲染线程已启动")
            
            // 渲染循环
            while (isRendering) {
                
                // 检查是否需要初始化（在渲染线程中！）
                if (needsInit && !initialized) {
                    val surface = currentSurface
                    if (surface != null && surface.isValid) {
                        Log.d(TAG, "在渲染线程中初始化EGL...")
                        if (renderer.nativeInit(surface)) {
                            initialized = true
                            needsInit = false
                            Log.d(TAG, "EGL初始化成功（渲染线程）")
                        } else {
                            Log.e(TAG, "EGL初始化失败")
                            break
                        }
                    }
                }
                
                // 检查是否需要更新尺寸
                if (needsResize && initialized) {
                    renderer.nativeResize(surfaceWidth, surfaceHeight)
                    needsResize = false
                    Log.d(TAG, "视口已更新: ${surfaceWidth}x${surfaceHeight}（渲染线程）")
                }
                
                // 渲染一帧
                if (initialized) {
                    renderer.nativeRender()
                }
                
                // 控制帧率：约60 FPS
                try {
                    sleep(16)
                } catch (e: InterruptedException) {
                    break
                }
            }
            
            // 清理资源（必须在同一个线程中！）
            if (initialized) {
                Log.d(TAG, "在渲染线程中清理EGL...")
                renderer.nativeCleanup()
                initialized = false
            }
            
            Log.d(TAG, "渲染线程已结束")
        }
    }
}
