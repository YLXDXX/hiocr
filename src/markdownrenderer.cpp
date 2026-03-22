#include "markdownrenderer.h"
#include "markdownbridge.h"
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
    // 主布局：垂直排列，无外边距，控件间小间距
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);   // 去除四周空白
    layout->setSpacing(2);                    // 控件之间间距 2 像素，保持紧凑

    // 标题标签：垂直方向使用 Minimum 策略，不占用多余高度
    QLabel* titleLabel = new QLabel("Markdown 渲染结果:", this);
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    titleLabel->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(titleLabel);

    // 创建 WebEngineView
    m_view = new QWebEngineView(this);
    m_view->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    connect(m_view, &QWebEngineView::loadFinished, this, &MarkdownRenderer::onPageLoaded);

    // 设置 WebChannel 和 Bridge
    m_bridge = new MarkdownBridge(this);
    m_channel = new QWebChannel(this);
    m_channel->registerObject("bridge", m_bridge);
    m_view->page()->setWebChannel(m_channel);

    // 加载 HTML 模板（从资源文件）
    QFile htmlFile(":/resources/markdown_renderer.html");
    if (htmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&htmlFile);
        QString htmlContent = stream.readAll();
        m_view->setHtml(htmlContent, QUrl("qrc:/"));
    } else {
        m_view->setHtml("<body>Error loading renderer</body>");
    }

    // WebEngineView 垂直方向可拉伸，以填充剩余空间
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


