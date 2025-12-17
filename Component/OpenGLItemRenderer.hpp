#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <memory>

#include "irenderer.hpp"
#include "render_context.hpp"
#include "render_config.hpp"

class OpenGLItem;

class OpenGLItemRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions {
public:
    OpenGLItemRenderer( OpenGLItem* item );
    ~OpenGLItemRenderer() override;

    // QQuickFramebufferObject::Renderer 接口
    void render() override;     // 渲染到FBO
    QOpenGLFramebufferObject* createFramebufferObject( const QSize& size ) override;    // 创建FBO
    void synchronize( QQuickFramebufferObject* item ) override; // 同步数据

private:
    void initializeRenderer();
    void updateProjectMatrix(const QSize& size);
    void handleRenderError( RenderError error, const std::string& message );

    OpenGLItem* m_item;         // 指向OpenGLItem 用于访问配置和发射信号!!
    std::unique_ptr<IRenderer> m_renderer;
    RenderConfig m_config;
    QMatrix4x4 m_projectMatrix;
    quint64 m_frameNumber;
    bool m_rendererInitialized;
    QString m_currentRendererType;
};
