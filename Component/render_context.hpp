#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>

struct ViewportSize {
    int width;
    int height;
    
    ViewportSize(int w = 0, int h = 0) : width(w), height(h) {}
};

class RenderContext {
public:
    RenderContext( const ViewportSize& viewportSize,
                  const glm::mat4& projectionMatrix,
                  float deltaTime = 0.0f )
    : m_viewportSize(viewportSize)
    , m_projectionMatrix(projectionMatrix)
    , m_deltaTime(deltaTime)
    , m_frameNumber(0)
    {}

    // Getters
    ViewportSize viewportSize() const { return m_viewportSize; }
    int width() const { return m_viewportSize.width; }
    int height() const { return m_viewportSize.height; }

    glm::mat4 projectionMatrix() const { return m_projectionMatrix; }

    float deltaTime() const { return m_deltaTime; }
    uint64_t frameNumer() const { return m_frameNumber; }

    // 创建新的上下文(不可变模式)
    // 上下文一旦创建就不可修改 避免并发问题
    RenderContext withFrameNumber( uint64_t frame ) const {
        RenderContext ctx = *this;
        ctx.m_frameNumber = frame;
        return ctx;
    }

    RenderContext withDeltaTime( float dt ) const {
        RenderContext ctx = *this;
        ctx.m_deltaTime = dt;
        return ctx;
    }

private:
    ViewportSize m_viewportSize;
    glm::mat4 m_projectionMatrix;
    float m_deltaTime;
    uint64_t m_frameNumber;
};
