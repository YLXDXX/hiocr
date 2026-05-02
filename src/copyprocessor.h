// src/copyprocessor.h
#ifndef COPYPROCESSOR_H
#define COPYPROCESSOR_H

#include <QObject>
#include <QProcess>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>
#include "contenttype.h"
#include "settingsmanager.h"

class CopyProcessor : public QObject
{
    Q_OBJECT
public:
    explicit CopyProcessor(QObject *parent = nullptr);

    void processAndCopy(const QString& text, ContentType originalType);
    void manualProcess(const QString& text, ContentType type);

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
    QString m_serviceName;
};

// --- Implementation ---

inline CopyProcessor::CopyProcessor(QObject *parent)
: QObject(parent)
{
}

inline void CopyProcessor::setServiceName(const QString& name)
{
    m_serviceName = name;
}

inline void CopyProcessor::processAndCopy(const QString& text, ContentType originalType)
{
    m_originalTextForFallback = text;

    if (!SettingsManager::instance()->autoExternalProcessBeforeCopy()) {
        QGuiApplication::clipboard()->setText(text);
        emit finished(text);
        return;
    }

    QString command = getFinalCommand(originalType, text);

    if (command.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
        emit finished(text);
        return;
    }

    executeCommand(command, text);
}

inline void CopyProcessor::manualProcess(const QString& text, ContentType type)
{
    m_originalTextForFallback = text;
    QString command;

    SettingsManager* s = SettingsManager::instance();
    switch(type) {
        case ContentType::Text: command = s->textProcessorCommand(); break;
        case ContentType::Formula: command = s->formulaProcessorCommand(); break;
        case ContentType::Table: command = s->tableProcessorCommand(); break;
        case ContentType::PureMath: command = s->pureMathProcessorCommand(); break;
        default: break;
    }

    if (command.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
        emit finished(text);
        return;
    }

    executeCommand(command, text);
}

inline QString CopyProcessor::getFinalCommand(ContentType originalType, const QString& text)
{
    SettingsManager* s = SettingsManager::instance();

    if (isPureMathContent(text)) {
        if (s->pureMathProcessorEnabled()) {
            QString cmd = s->pureMathProcessorCommand();
            if (!cmd.isEmpty()) return cmd;
        }
    }

    switch (originalType) {
        case ContentType::Formula: {
            if (s->formulaProcessorEnabled()) {
                QString cmd = s->formulaProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
        case ContentType::Table: {
            if (s->tableProcessorEnabled()) {
                QString cmd = s->tableProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
        case ContentType::Text: {
            if (s->textProcessorEnabled()) {
                QString cmd = s->textProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
        case ContentType::MixedContent: {
            if (s->textProcessorEnabled()) {
                QString cmd = s->textProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
        case ContentType::PureMath: {
            if (s->pureMathProcessorEnabled()) {
                QString cmd = s->pureMathProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
    }

    return QString();
}

inline bool CopyProcessor::isPureMathContent(const QString& text) const
{
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return false;

    if (trimmed.contains("<math")) return false;

    bool isDisplay = (trimmed.startsWith("$$") && trimmed.endsWith("$$")) ||
    (trimmed.startsWith("\\[") && trimmed.endsWith("\\]"));

    bool isInline = (trimmed.startsWith("$") && trimmed.endsWith("$") && !trimmed.startsWith("$$")) ||
    (trimmed.startsWith("\\(") && trimmed.endsWith("\\)"));

    if (isDisplay || isInline) {
        if (trimmed.contains("\n\n")) return false;
        if (trimmed.startsWith("$$") && trimmed.count("$$") > 2) return false;
        if (trimmed.startsWith("\\[") && (trimmed.count("\\[") > 1 || trimmed.count("\\]") > 1)) return false;
        return true;
    }
    return false;
}

inline void CopyProcessor::executeCommand(const QString& command, const QString& text)
{
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess->deleteLater();
    }

    m_currentProcess = new QProcess(this);
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CopyProcessor::onProcessFinished);

    QString fullCommand = command;
    if (!m_serviceName.isEmpty()) {
        fullCommand += " --mode-name \"" + m_serviceName + "\"";
    }

    m_currentProcess->startCommand(fullCommand);
    m_currentProcess->write(text.toUtf8());
    m_currentProcess->closeWriteChannel();
}

inline void CopyProcessor::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString result;

    if (status == QProcess::NormalExit && exitCode == 0) {
        result = QString::fromUtf8(process->readAllStandardOutput());
        if (result.isEmpty()) result = m_originalTextForFallback;
    } else {
        QString err = process->readAllStandardError();
        qWarning() << "Copy processor failed:" << err;
        result = m_originalTextForFallback;
        emit error("External processor failed: " + err);
    }

    QGuiApplication::clipboard()->setText(result);
    emit finished(result);

    process->deleteLater();
    m_currentProcess = nullptr;
}

#endif // COPYPROCESSOR_H
