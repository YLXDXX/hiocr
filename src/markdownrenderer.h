#ifndef MARKDOWNRENDERER_H
#define MARKDOWNRENDERER_H

#include <QWidget>
#include <QWebEngineView>
#include <QWebChannel>
#include <QTimer> // 【新增】

class MarkdownBridge;

class MarkdownRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownRenderer(QWidget* parent = nullptr);

    void setMarkdown(const QString& markdown);   // 设置并渲染最终 Markdown（非流式用）

    // 【新增】流式模式接口
    void startStreaming();                       // 开始流式模式：启动定时器
    void appendStreamContent(const QString& delta); // 追加增量文本
    void stopStreaming();                        // 结束流式模式：强制渲染最终结果

signals:
    void pageLoaded(bool ok);

private slots:
    void onPageLoaded(bool ok);
    void renderMarkdown(const QString& markdown);
    void applyFontSize(int size);

    // 【新增】定时器触发槽
    void onRenderTimerTimeout();

private:
    void setupUi();

    QWebEngineView* m_view;
    QWebChannel* m_channel;
    MarkdownBridge* m_bridge;
    bool m_loaded;
    QString m_pendingMarkdown;

    // 【新增】流式渲染相关成员
    QTimer* m_renderTimer = nullptr;    // 渲染节流定时器
    QString m_streamBuffer;             // 流式文本缓冲区
    bool m_isStreaming = false;         // 是否处于流式模式
};

#endif // MARKDOWNRENDERER_H
