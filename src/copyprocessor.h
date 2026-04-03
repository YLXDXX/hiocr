// src/copyprocessor.h
#ifndef COPYPROCESSOR_H
#define COPYPROCESSOR_H

#include <QObject>
#include <QProcess>

// 定义内容类型，与 MarkdownCopyBar 解耦，供全局使用
enum class ContentType {
    Text,           // 纯文本
    Formula,        // 单个公式
    Table,          // 表格
    MixedContent,   // 混合内容
    PureMath        // 纯数学公式 (TODO 逻辑预留)
};

class CopyProcessor : public QObject
{
    Q_OBJECT
public:
    explicit CopyProcessor(QObject *parent = nullptr);

    // 处理文本并写入剪贴板，根据类型自动选择脚本
    void processAndCopy(const QString& text, ContentType type);

    // 直接执行指定命令（用于手动触发外部处理）
    void executeCommand(const QString& command, const QString& text);

signals:
    void finished(const QString& result);
    void error(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    QProcess* m_currentProcess = nullptr;
    ContentType m_currentType;
    QString m_originalTextForFallback;
};

#endif // COPYPROCESSOR_H
