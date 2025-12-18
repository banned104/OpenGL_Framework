package com.example.androidopengles

import android.view.Surface

/**
 * NativeRenderer - JNI桥接类
 * 
 * 这个类负责与C++编写的native_renderer.so库进行通信。
 * 它声明了一系列"external"函数，这些函数的实际实现在C++代码中。
 * 
 * 工作原理：
 * 1. companion object中的init块会在类加载时执行
 * 2. System.loadLibrary("native_renderer")会加载libnative_renderer.so库
 * 3. external关键字声明的函数会自动链接到C++中对应的JNI函数
 */
class NativeRenderer {
    
    /**
     * 初始化渲染器
     * 
     * @param surface Android的Surface对象，用于创建EGL渲染表面
     * @return true表示初始化成功，false表示失败
     * 
     * 对应C++函数：Java_com_example_opengl_NativeRenderer_nativeInit
     * 注意：C++中的包名是com.example.opengl，需要修改C++代码或者这里的包名
     */
    external fun nativeInit(surface: Surface): Boolean
    
    /**
     * 渲染一帧
     * 
     * 调用此函数会执行一次完整的渲染循环：
     * 1. 清除屏幕
     * 2. 绘制三角形
     * 3. 交换前后缓冲区
     * 
     * 对应C++函数：Java_com_example_opengl_NativeRenderer_nativeRender
     */
    external fun nativeRender()
    
    /**
     * 处理Surface尺寸变化
     * 
     * 当屏幕旋转或窗口大小改变时调用此函数。
     * 它会更新OpenGL的视口(viewport)和投影矩阵。
     * 
     * @param width 新的宽度（像素）
     * @param height 新的高度（像素）
     * 
     * 对应C++函数：Java_com_example_opengl_NativeRenderer_nativeResize
     */
    external fun nativeResize(width: Int, height: Int)
    
    /**
     * 清理资源
     * 
     * 释放所有OpenGL资源、EGL上下文和Surface。
     * 应该在Activity销毁或Surface销毁时调用。
     * 
     * 对应C++函数：Java_com_example_opengl_NativeRenderer_nativeCleanup
     */
    external fun nativeCleanup()
    
    /**
     * 获取渲染器名称
     * 
     * @return 当前渲染器的名称字符串
     * 
     * 对应C++函数：Java_com_example_opengl_NativeRenderer_nativeGetRendererName
     */
    external fun nativeGetRendererName(): String
    
    /**
     * companion object - Kotlin的静态成员区域
     * 
     * 相当于Java中的static块和static方法。
     * init块在类第一次被使用时执行。
     */
    companion object {
        // 标记库是否已加载
        private var isLibraryLoaded = false
        
        /**
         * init块 - 类加载时执行
         * 
         * System.loadLibrary("main_opengl")会：
         * 1. 在jniLibs目录下查找libmain_opengl.so
         * 2. 加载这个共享库到内存
         * 3. 建立JNI函数的链接
         */
        init {
            try {
                System.loadLibrary("main_opengl")
                isLibraryLoaded = true
                android.util.Log.i("NativeRenderer", "main_opengl库加载成功")
            } catch (e: UnsatisfiedLinkError) {
                // 如果.so文件不存在或架构不匹配，会抛出此异常
                isLibraryLoaded = false
                android.util.Log.e("NativeRenderer", "加载main_opengl库失败: ${e.message}")
            }
        }
        
        /**
         * 检查库是否已成功加载
         */
        fun isReady(): Boolean = isLibraryLoaded
    }
}
