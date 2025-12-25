#pragma once
#include "irenderer.hpp"
#include <memory>

// 根据编译宏包含对应的渲染器
#ifdef USE_TRIANGLE_RENDER
    #include "triangle_render.hpp"
#endif

#ifdef USE_CUBE_RENDER
    #include "cube_render.hpp"
#endif

enum class RenderType {
    Triangle,
    Cube,
};

class RenderFactory {
public:
    static std::unique_ptr<IRenderer> create(RenderType type) {
        switch (type) {
        #ifdef USE_TRIANGLE_RENDER
        case RenderType::Triangle:
            return std::make_unique<TriangleRender>();
        #endif
        
        #ifdef USE_CUBE_RENDER
        case RenderType::Cube:
            return std::make_unique<CubeRender>();
        #endif
        
        default:
            return nullptr;
        }
    }

    static std::unique_ptr<IRenderer> create(const std::string& typeName) {
        #ifdef USE_TRIANGLE_RENDER
        if (typeName == "triangle") {
            return create(RenderType::Triangle);
        }
        #endif
        
        #ifdef USE_CUBE_RENDER
        if (typeName == "cube") {
            return create(RenderType::Cube);
        }
        #endif
        
        return nullptr;
    }

private:
    RenderFactory() = delete;
    RenderFactory(const RenderFactory&) = delete;
    RenderFactory& operator=(const RenderFactory&) = delete;
};
