#include "markdownrenderer.h"
#include "markdownbridge.h"
#include "settingsmanager.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QWebEngineSettings>


MarkdownRenderer::MarkdownRenderer(QWidget* parent)
: QWidget(parent), m_loaded(false), m_isStreaming(false)
{
    setupUi();

    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(300);
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

    m_bridge = new MarkdownBridge(this);
    m_bridge->setMathFont(SettingsManager::instance()->mathFont());

    connect(SettingsManager::instance(), &SettingsManager::mathFontChanged, m_bridge, &MarkdownBridge::setMathFont);
    applyFontSize(SettingsManager::instance()->rendererFontSize());
    connect(SettingsManager::instance(), &SettingsManager::rendererFontSizeChanged, this, &MarkdownRenderer::applyFontSize);

    m_channel = new QWebChannel(this);
    m_channel->registerObject("bridge", m_bridge);
    m_view->page()->setWebChannel(m_channel);

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

// 【新增】设置服务名称
void MarkdownRenderer::setServiceName(const QString& name)
{
    if (m_bridge) {
        m_bridge->setServiceName(name);
    }
}

void MarkdownRenderer::setMarkdown(const QString& markdown)
{
    if (m_isStreaming) {
        stopStreaming();
    }

    if (m_loaded)
        renderMarkdown(markdown);
    else
        m_pendingMarkdown = markdown;
}

void MarkdownRenderer::startStreaming()
{
    m_streamBuffer.clear();
    m_isStreaming = true;
    m_renderTimer->start();
}

void MarkdownRenderer::appendStreamContent(const QString& delta)
{
    if (!m_isStreaming) {
        startStreaming();
    }
    m_streamBuffer.append(delta);
}

void MarkdownRenderer::stopStreaming()
{
    if (!m_isStreaming) return;
    m_isStreaming = false;
    m_renderTimer->stop();
    renderMarkdown(m_streamBuffer);
    m_streamBuffer.clear();
}

void MarkdownRenderer::onRenderTimerTimeout()
{
    if (m_streamBuffer.isEmpty()) return;
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
    if (m_view && m_loaded) {
        QString js = QString("document.body.style.fontSize = '%1px';").arg(size);
        m_view->page()->runJavaScript(js);
    }
}
