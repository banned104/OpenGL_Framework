#include "opengl_item.hpp"
#include "global_macro.hpp"
#include "render_factory.hpp"
#include <QQuickWindow>
#include <string>



OpenGLItem::OpenGLItem()
    : m_fps(0)
    , m_lastTime( QTime::currentTime() )
    , m_rendererType( "triangle" )
    , m_frameNumer(0)
    , m_rendererInitialized( false )
{
    m_config = RenderConfig::createTriangleConfig();

    // FBO纹理需要垂直翻转
    setMirrorVertically(true);
}

OpenGLItem::~OpenGLItem() {}

QQuickFramebufferObject::Renderer* OpenGLItem::createRenderer() const {
    // 创建渲染器 -> 要在渲染线程中调用!!!!!!!!!
    // 但 OpenGLItemRenderer 构造函数需要 OpenGLItem*（非 const）
    return new OpenGLItemRenderer( const_cast<OpenGLItem*>(this) );

}

void OpenGLItem::setFps( int fps ) {
    if ( fps == m_fps ) return;
    m_fps = fps;
    emit fpsChanged();
}

void OpenGLItem::setRenderType( const QString& type ) {
    if ( type == m_rendererType ) return;

    m_rendererType = type;

    emit renderTypeChanged();
    update();
}

void OpenGLItem::setRenderConfig( const RenderConfig& config ) {
    m_config = config;
    update();
}

/*


void OpenGLItem::setRenderer( std::unique_ptr<IRenderer> renderer ) {
    if ( m_renderer ) {
        m_renderer->cleanup();
    }

    m_renderer = std::move( renderer );
    m_rendererInitialized = false;
}




void OpenGLItem::cleanup() {
    if ( m_renderer ) {
        m_renderer->cleanup();
        m_renderer.reset();
    }
    m_rendererInitialized = false;
}

void OpenGLItem::timerEvent( QTimerEvent* e ) {
    Q_UNUSED( e );
    if ( window() ) {
        window()->update();
    }
}

void OpenGLItem::createRenderer() {
    m_renderer = RenderFactory::create( m_rendererType.toStdString() );

    if ( !m_renderer ) {
        qWarning() << "Failed to create: " << m_rendererType;
    }
}


*/




