#include "OpenGLItemRenderer.hpp"
#include "opengl_item.hpp"
#include "render_factory.hpp"
#include <QOpenGLFramebufferObject>
#include <QDebug>

OpenGLItemRenderer::OpenGLItemRenderer( OpenGLItem* item )
    : m_item(item)
    , m_frameNumber(0)
    , m_rendererInitialized(false)
{
    initializeOpenGLFunctions();
    m_config = item->config();
    m_currentRendererType = item->renderType();
}

OpenGLItemRenderer::~OpenGLItemRenderer() {
    if ( m_renderer ) {
        m_renderer->cleanup();
    }
}

void OpenGLItemRenderer::render() {
    // 渲染线程中 每一帧都调用
    if ( !m_renderer ) {
        m_renderer = RenderFactory::create( m_currentRendererType.toStdString() );
        if ( m_renderer ) {
            initializeRenderer();
        } else {

        }
    }

    if ( m_renderer && m_rendererInitialized ) {
        // 获取当前渲染的FBO尺寸
        QSize fboSize = framebufferObject()->size();

        // 创建渲染上下文
        RenderContext context(
            fboSize,
            m_projectMatrix,
            0.0f
        );
        context = context.withFrameNumber(m_frameNumber++);

        // 执行渲染
        m_renderer->render(context);

    }

    // 触发下一帧 "Call this function when the FBO should be renderered angain."
    update();
}

// 第一帧的时候调用  如果设置 QQuickFramebufferObject::textureFollowsItemSize(true); 就会在每次组件大小改变时调用这个 createFrameBuffer
QOpenGLFramebufferObject* OpenGLItemRenderer::createFramebufferObject( const QSize& size  ) {
    // 创建FBO 当尺寸变化时调用
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment( QOpenGLFramebufferObject::CombinedDepthStencil );
    format.setSamples(4);   // 4x MSAA抗锯齿

    // non-static 但是设置这个 "纹理跟随Item大小变化" 可以让组件大小变化时 FBO渲染出来的纹理大小随之改变
    // QQuickFramebufferObject::textureFollowsItemSize(true);


    // 更新投影矩阵
    updateProjectMatrix(size);

    // 通知渲染器尺寸变化
    if ( m_renderer && m_rendererInitialized ) {
        m_renderer->resize( size.width(), size.height() );
    }

    return new QOpenGLFramebufferObject( size, format );
}

void OpenGLItemRenderer::synchronize( QQuickFramebufferObject* item ) {
    // 从GUI线程同步数据到渲染线程
    OpenGLItem* glItem = static_cast<OpenGLItem*>(item);

    // 同步配置
    if ( m_config.vertexShaderPath() != glItem->config().vertexShaderPath()
        || m_config.fragmentShaderPath() != glItem->config().fragmentShaderPath() )
    {
        // 重新初始化渲染器
        if ( m_renderer ) {
            m_renderer->cleanup();
            initializeRenderer();
        }
    }

    // 同步渲染器类型
    if ( m_currentRendererType != glItem->renderType() ) {
        m_currentRendererType = glItem->renderType();

        // 重新创建渲染器
        if ( m_renderer ) {
            m_renderer->cleanup();
            m_renderer.reset();
        }
        m_rendererInitialized = false;
        // 同步配置 新渲染器需要配置
        m_config = glItem->config();
    } else {
        RenderConfig newConfig = glItem->config();

        // 检查各个配置是否变化
        if ( m_config.vertexShaderPath() != newConfig.vertexShaderPath()
            || m_config.fragmentShaderPath() != newConfig.fragmentShaderPath()
            || m_config.vertexData().size() != newConfig.vertexShaderPath().size() )
        {
            m_config = newConfig;

            // 配置变化 重新初始化
            if ( m_renderer ) {
                m_renderer->cleanup();
                m_rendererInitialized = false;
                initializeRenderer();
            }
        }
    }
}

void OpenGLItemRenderer::initializeRenderer() {
    if ( !m_renderer ) return;

    // 设置错误回调
    m_renderer->setErrorCallback( [this]( RenderError error, const std::string& msg ){
        handleRenderError( error, msg );
    } );

    // 初始化渲染器
    if ( !m_renderer->initialize(m_config) ) {
        qWarning() << "Failed to initialize renderer";
        m_renderer.reset();
        m_rendererInitialized = false;
    } else {
        m_rendererInitialized = true;
    }
}


// 修改组件宽高之后重新计算矩阵 矩阵会在每一帧参与计算 RenderContext
void OpenGLItemRenderer::updateProjectMatrix( const QSize& size ) {
    if ( size.width() <= 0 || size.height() <= 0 ) {
        return;
    }

    qreal aspect = qreal(size.width()) / qreal(size.height());
    m_projectMatrix.setToIdentity();
    m_projectMatrix.perspective(30.0f, aspect, 3.0f, 10.0f);
}

void OpenGLItemRenderer::handleRenderError( RenderError error, const std::string& msg ) {
    QString errorMsg = QString::fromStdString(msg);
    qWarning() << "Render Error: " << errorMsg;

    // 不能直接发射信号 线程不同
    QMetaObject::invokeMethod( m_item, "renderError",
                               Qt::QueuedConnection,
                              Q_ARG( QString, errorMsg )
                              );
}

