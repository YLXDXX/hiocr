#include "markdownrenderer.h"
#include "markdownbridge.h"
#include "settingsmanager.h" // 引入设置管理器

#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QWebEngineSettings>


MarkdownRenderer::MarkdownRenderer(QWidget* parent)
: QWidget(parent), m_loaded(false), m_isStreaming(false)
{
    setupUi();

    // 【新增】初始化定时器
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(300); // 设置刷新间隔为 300ms (可调整)
    connect(m_renderTimer, &QTimer::timeout, this, &MarkdownRenderer::onRenderTimerTimeout);
}

void MarkdownRenderer::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    QLabel* titleLabel = new QLabel("Markdown 渲染结果:", this);
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    titleLabel->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(titleLabel);

    m_view = new QWebEngineView(this);
    m_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    connect(m_view, &QWebEngineView::loadFinished, this, &MarkdownRenderer::onPageLoaded);

    // 设置 WebChannel 和 Bridge
    m_bridge = new MarkdownBridge(this);

    // 【新增】初始化字体设置
    m_bridge->setMathFont(SettingsManager::instance()->mathFont());

    // 【新增】连接信号，当设置中的字体改变时，更新 Bridge
    connect(SettingsManager::instance(), &SettingsManager::mathFontChanged, m_bridge, &MarkdownBridge::setMathFont);

    // 【新增】初始化字体大小
    applyFontSize(SettingsManager::instance()->rendererFontSize());

    // 【新增】连接字体大小变化信号
    connect(SettingsManager::instance(), &SettingsManager::rendererFontSizeChanged, this, &MarkdownRenderer::applyFontSize);

    m_channel = new QWebChannel(this);
    m_channel->registerObject("bridge", m_bridge);
    m_view->page()->setWebChannel(m_channel);

    // 加载 HTML 模板
    QFile htmlFile(":/resources/markdown_renderer.html");
    if (htmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&htmlFile);
        QString htmlContent = stream.readAll();
        m_view->setHtml(htmlContent, QUrl("qrc:/"));
    } else {
        m_view->setHtml("<body>Error loading renderer</body>");
    }

    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_view);
}

void MarkdownRenderer::setMarkdown(const QString& markdown)
{
    // 如果正在流式传输，外部强制调用 setMarkdown，则视为中断流式，停止定时器
    if (m_isStreaming) {
        stopStreaming();
    }

    if (m_loaded)
        renderMarkdown(markdown);
    else
        m_pendingMarkdown = markdown;
}


// 【新增】开始流式模式
void MarkdownRenderer::startStreaming()
{
    m_streamBuffer.clear();
    m_isStreaming = true;
    m_renderTimer->start(); // 启动定时器
}

// 【新增】追加内容
void MarkdownRenderer::appendStreamContent(const QString& delta)
{
    if (!m_isStreaming) {
        // 如果未显式调用 startStreaming，这里自动开启（容错）
        startStreaming();
    }

    m_streamBuffer.append(delta);

    // 注意：这里不立即渲染，等待定时器触发
    // 但我们可以选择立即更新纯文本到界面的某处（如果需要），
    // 不过 MainWindow 的源码编辑器已经做了这件事，这里只管 Web 渲染。
}

// 【新增】停止流式模式
void MarkdownRenderer::stopStreaming()
{
    if (!m_isStreaming) return;

    m_isStreaming = false;
    m_renderTimer->stop();

    // 强制立即渲染最后一次缓冲区内容，确保结果完整
    renderMarkdown(m_streamBuffer);
    m_streamBuffer.clear();
}

// 【新增】定时器回调
void MarkdownRenderer::onRenderTimerTimeout()
{
    if (m_streamBuffer.isEmpty()) return;

    // 渲染当前缓冲区的内容
    // 注意：这里每次都全量渲染 m_streamBuffer，以保证 Markdown 语法的正确性（如闭合标签）
    renderMarkdown(m_streamBuffer);
}

void MarkdownRenderer::renderMarkdown(const QString& markdown)
{
    if (m_bridge) {
        m_bridge->setMarkdown(markdown);
    }
}

void MarkdownRenderer::onPageLoaded(bool ok)
{
    m_loaded = ok;
    emit pageLoaded(ok);

    if (ok) {
        applyFontSize(SettingsManager::instance()->rendererFontSize());
        if (!m_pendingMarkdown.isEmpty()) {
            renderMarkdown(m_pendingMarkdown);
            m_pendingMarkdown.clear();
        }
    }
}

void MarkdownRenderer::applyFontSize(int size)
{
    // 如果页面已加载，通过 JS 动态修改样式
    if (m_view && m_loaded) {
        QString js = QString("document.body.style.fontSize = '%1px';").arg(size);
        m_view->page()->runJavaScript(js);
    }
}



