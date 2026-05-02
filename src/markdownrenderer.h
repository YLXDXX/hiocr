#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QWebEngineSettings>

#include "markdownbridge.h"
#include "settingsmanager.h"

class MarkdownRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownRenderer(QWidget* parent = nullptr);

    void setMarkdown(const QString& markdown);

    void startStreaming();
    void appendStreamContent(const QString& delta);
    void stopStreaming();

    void setServiceName(const QString& name);

signals:
    void pageLoaded(bool ok);

private slots:
    void onPageLoaded(bool ok);
    void renderMarkdown(const QString& markdown);
    void applyFontSize(int size);
    void onRenderTimerTimeout();

private:
    void setupUi();

    QWebEngineView* m_view;
    QWebChannel* m_channel;
    MarkdownBridge* m_bridge;
    bool m_loaded;
    QString m_pendingMarkdown;

    QTimer* m_renderTimer = nullptr;
    QString m_streamBuffer;
    bool m_isStreaming = false;
};

// --- Implementation ---

inline MarkdownRenderer::MarkdownRenderer(QWidget* parent)
: QWidget(parent), m_loaded(false), m_isStreaming(false)
{
    setupUi();

    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(300);
    connect(m_renderTimer, &QTimer::timeout, this, &MarkdownRenderer::onRenderTimerTimeout);
}

inline void MarkdownRenderer::setupUi()
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

inline void MarkdownRenderer::setServiceName(const QString& name)
{
    if (m_bridge) {
        m_bridge->setServiceName(name);
    }
}

inline void MarkdownRenderer::setMarkdown(const QString& markdown)
{
    if (m_isStreaming) {
        stopStreaming();
    }

    if (m_loaded)
        renderMarkdown(markdown);
    else
        m_pendingMarkdown = markdown;
}

inline void MarkdownRenderer::startStreaming()
{
    m_streamBuffer.clear();
    m_isStreaming = true;
    m_renderTimer->start();
}

inline void MarkdownRenderer::appendStreamContent(const QString& delta)
{
    if (!m_isStreaming) {
        startStreaming();
    }
    m_streamBuffer.append(delta);
}

inline void MarkdownRenderer::stopStreaming()
{
    if (!m_isStreaming) return;
    m_isStreaming = false;
    m_renderTimer->stop();
    renderMarkdown(m_streamBuffer);
    m_streamBuffer.clear();
}

inline void MarkdownRenderer::onRenderTimerTimeout()
{
    if (m_streamBuffer.isEmpty()) return;
    renderMarkdown(m_streamBuffer);
}

inline void MarkdownRenderer::renderMarkdown(const QString& markdown)
{
    if (m_bridge) {
        m_bridge->setMarkdown(markdown);
    }
}

inline void MarkdownRenderer::onPageLoaded(bool ok)
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

inline void MarkdownRenderer::applyFontSize(int size)
{
    if (m_view && m_loaded) {
        QString js = QString("document.body.style.fontSize = '%1px';").arg(size);
        m_view->page()->runJavaScript(js);
    }
}

#endif // MARKDOWNRENDERER_H
