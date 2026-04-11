// src/copyprocessor.h
#ifndef COPYPROCESSOR_H
#define COPYPROCESSOR_H

#include <QObject>
#include <QProcess>

enum class ContentType {
    Text,
    Formula,
    Table,
    MixedContent,
    PureMath
};

class CopyProcessor : public QObject
{
    Q_OBJECT
public:
    explicit CopyProcessor(QObject *parent = nullptr);

    void processAndCopy(const QString& text, ContentType originalType);
    void manualProcess(const QString& text, ContentType type);

    // 【新增】设置当前服务名称，调用外部脚本时附加 --mode-name 参数
    void setServiceName(const QString& name);

signals:
    void finished(const QString& result);
    void error(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    bool isPureMathContent(const QString& text) const;
    QString getFinalCommand(ContentType originalType, const QString& text);
    void executeCommand(const QString& command, const QString& text);

    QProcess* m_currentProcess = nullptr;
    QString m_originalTextForFallback;

    // 【新增】当前服务名称，用于传递给外部脚本
    QString m_serviceName;
};

#endif // COPYPROCESSOR_H
