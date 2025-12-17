#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include "OpenGLItemRenderer.hpp"
#include <QTime>
#include <QQuickFramebufferObject>
#include <QBasicTimer>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class OpenGLItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged FINAL)
    Q_PROPERTY(QString renderType READ renderType WRITE setRenderType NOTIFY renderTypeChanged FINAL)

public:
    OpenGLItem();
    ~OpenGLItem() override;

    // QQuickFramebufferObject 接口
    Renderer* createRenderer() const override;  // 关键方法

    // 属性访问
    int fps() const { return m_fps; }
    void setFps( int f );

    QString renderType() const { return m_rendererType; }
    void setRenderType( const QString& type );

    // 依赖注入接口
    void setRenderer(std::unique_ptr<IRenderer> renderer);

    // 配置接口
    RenderConfig config() const {return m_config;}
    void setRenderConfig( const RenderConfig& config );

signals:
    void fpsChanged();
    void renderTypeChanged();
    void renderError( const QString& message );

private:
    RenderConfig m_config;

    int m_fps;
    QTime m_lastTime;
    QBasicTimer m_timer;
    QString m_rendererType;
    quint64 m_frameNumer;

    bool m_rendererInitialized;

    friend class OpenGLItemRenderer;        // 允许 Renderer 访问私有成员
};

#endif
