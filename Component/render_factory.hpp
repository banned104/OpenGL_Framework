#pragma once
#include "irenderer.hpp"
#include "triangle_render.hpp"
#include <memory>

enum class RenderType {
    Triangle,
    Cube,
    Custom,
};


class RenderFactory {
public:
    static std::unique_ptr<IRenderer> create( RenderType type ) {
        switch (type) {
        case RenderType::Triangle:
            return std::make_unique<TriangleRender>();
            break;
        default:
            return nullptr;
            break;
        }
    }

    static std::unique_ptr<IRenderer> create( const std::string& typeName ) {
        if ( typeName == "triangle" ) {
            return create( RenderType::Triangle );
        } else {
            return nullptr;
        }
    }
private:
    // 禁止实例化
    RenderFactory() = delete;
    RenderFactory(const RenderFactory&) = delete;
    RenderFactory& operator=(const RenderFactory&) = delete;
};
