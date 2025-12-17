#pragma once

#include <QMatrix4x4>
#include <QSize>

class RenderContext {
public:
    RenderContext( const QSize& viewportSize,
                  const QMatrix4x4& projectionMatrix,
                  float deltaTime = 0.0f )
    : m_viewportSize(viewportSize)
    , m_projectionMatrix(projectionMatrix)
    , m_deltaTime(deltaTime)
    , m_frameNumber(0)
    {}

    // Getters
    QSize viewportSize() const { return m_viewportSize; }
    int width() const { return m_viewportSize.width(); }
    int height() const { return m_viewportSize.height(); }

    QMatrix4x4 projectionMatrix() const { return m_projectionMatrix; }

    float deltaTime() const { return m_deltaTime; }
    quint64 frameNumer() const { return m_frameNumber; }

    // 创建新的上下文(不可变模式)
    // 上下文一旦创建就不可修改 避免并发问题
    RenderContext withFrameNumber( quint64 frame ) const {
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
    QSize m_viewportSize;
    QMatrix4x4 m_projectionMatrix;
    float m_deltaTime;
    quint64 m_frameNumber;
};
