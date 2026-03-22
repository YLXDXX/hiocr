#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>

class MarkdownBridge;

class MarkdownRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownRenderer(QWidget* parent = nullptr);

    void setMarkdown(const QString& markdown);   // 设置要渲染的 Markdown 文本

signals:
    void pageLoaded(bool ok);   // 页面加载完成信号

private slots:
    void onPageLoaded(bool ok);
    void renderMarkdown(const QString& markdown);

private:
    void setupUi();

    QWebEngineView* m_view;
    QWebChannel* m_channel;
    MarkdownBridge* m_bridge;
    bool m_loaded;
    QString m_pendingMarkdown;
};

#endif // MARKDOWNRENDERER_H
