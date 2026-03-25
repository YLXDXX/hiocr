#include "markdownrenderer.h"
#include "markdownbridge.h"
#include "settingsmanager.h" // 引入设置管理器

#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QWebEngineSettings>


MarkdownRenderer::MarkdownRenderer(QWidget* parent)
: QWidget(parent), m_loaded(false)
{
    setupUi();
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
    if (m_loaded)
        renderMarkdown(markdown);
    else
        m_pendingMarkdown = markdown;
}

void MarkdownRenderer::onPageLoaded(bool ok)
{
    m_loaded = true;
    emit pageLoaded(ok);
    if (!m_pendingMarkdown.isEmpty()) {
        renderMarkdown(m_pendingMarkdown);
        m_pendingMarkdown.clear();
    }
}

void MarkdownRenderer::renderMarkdown(const QString& markdown)
{
    m_bridge->setMarkdown(markdown);
}


