@echo off
chcp 65001 >nul
echo ===============================================
echo   OpenGL Android .so 库编译脚本
echo   项目: 11_MainOpenGL
echo ===============================================
echo.

REM ============================================
REM 步骤1: 转换Shader文件为头文件 (Android ES版本)
REM ============================================
echo [1/4] 转换Shader为头文件 (Android ES版本)...

python shaders\Convert_GLSL_to_h.py shaders\triangle.vert.glsl shaders\triangle.vert.es.h --android
if errorlevel 1 (
    echo 错误: 顶点着色器转换失败！
    pause
    exit /b 1
)

python shaders\Convert_GLSL_to_h.py shaders\triangle.frag.glsl shaders\triangle.frag.es.h --android
if errorlevel 1 (
    echo 错误: 片段着色器转换失败！
    pause
    exit /b 1
)

echo    Shader转换完成!
echo.

REM ============================================
REM 步骤2: 配置CMake (使用NDK工具链)
REM ============================================
echo [2/4] 配置CMake...

REM 创建构建目录
if not exist build_android mkdir build_android

REM 配置说明:
REM   DCMAKE_MAKE_PROGRAM: 本地Ninja工具路径 (没有就到Github下载)
REM   DCMAKE_TOOLCHAIN_FILE: Android NDK工具链路径
REM   ANDROID_ABI: 目标架构 (arm64-v8a为64位ARM)
REM   ANDROID_PLATFORM: 最低Android API级别
REM   BUILD_AS_SHARED: 编译为共享库(.so)

cmake -S . -B build_android -G "Ninja" ^
    -DCMAKE_MAKE_PROGRAM=D:/MySoftwares/ninja-win/ninja.exe ^
    -DCMAKE_TOOLCHAIN_FILE=D:/MySoftwares/AndroidStudio_main/ndk/23.1.7779620/build/cmake/android.toolchain.cmake ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-21 ^
    -DBUILD_AS_SHARED=ON

if errorlevel 1 (
    echo 错误: CMake配置失败！
    pause
    exit /b 1
)

echo    CMake配置完成!
echo.

REM ============================================
REM 步骤3: 编译
REM ============================================
echo [3/4] 编译中...

cd build_android
ninja

if errorlevel 1 (
    cd ..
    echo 错误: 编译失败！
    pause
    exit /b 1
)

cd ..
echo    编译完成!
echo.

REM ============================================
REM 步骤4: 显示输出结果
REM ============================================
echo [4/4] 构建完成!
echo.
echo ===============================================
echo   构建输出:
echo ===============================================
dir /b build_android\*.so 2>nul
if errorlevel 1 (
    echo 警告: 未找到.so文件！
) else (
    echo.
    echo .so文件位置: %CD%\build_android\
)
echo ===============================================

REM ============================================
REM 可选: 复制.so文件到Android项目
REM ============================================
REM 取消下面的注释以启用自动复制功能

set TARGET_DIR=example\android\app\src\main\jniLibs\arm64-v8a

if not exist "%TARGET_DIR%" (
    echo 目标目录不存在: %TARGET_DIR%
    mkdir "%TARGET_DIR%"
)

echo 复制.so文件到 %TARGET_DIR%
copy /Y "build_android\*.so" "%TARGET_DIR%"
echo 复制完成!

pause

