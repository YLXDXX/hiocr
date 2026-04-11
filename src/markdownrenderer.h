#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTimer>

class MarkdownBridge;

class MarkdownRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownRenderer(QWidget* parent = nullptr);

    void setMarkdown(const QString& markdown);

    void startStreaming();
    void appendStreamContent(const QString& delta);
    void stopStreaming();

    // 【新增】设置当前服务名称，同步到 Bridge 供 JS 使用
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

#endif // MARKDOWNRENDERER_H
