// src/copyprocessor.h
#ifndef COPYPROCESSOR_H
#define COPYPROCESSOR_H

#include <QObject>
#include <QProcess>

enum class ContentType {
    Text,           // 纯文本
    Formula,        // 单个公式
    Table,          // 表格
    MixedContent,   // 混合内容
    PureMath        // 纯数学公式 (通过内容检测)
};

class CopyProcessor : public QObject
{
    Q_OBJECT
public:
    explicit CopyProcessor(QObject *parent = nullptr);

    // 处理文本并写入剪贴板，根据类型和设置自动选择脚本
    void processAndCopy(const QString& text, ContentType originalType);

    // 手动强制执行特定类型的处理 (用于快捷键调用)
    void manualProcess(const QString& text, ContentType type);

signals:
    void finished(const QString& result);
    void error(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    // 判断文本内容是否为纯数学公式
    bool isPureMathContent(const QString& text) const;
    // 根据逻辑获取最终要执行的命令
    QString getFinalCommand(ContentType originalType, const QString& text);

    void executeCommand(const QString& command, const QString& text);

    QProcess* m_currentProcess = nullptr;
    QString m_originalTextForFallback;
};

#endif // COPYPROCESSOR_H
