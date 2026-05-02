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
    QString runFormatter(const QString& text);

    QProcess* m_currentProcess = nullptr;
    QString m_originalTextForFallback;
    QString m_serviceName;
    bool m_applyFormatterAfterProcess = false;
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
    m_applyFormatterAfterProcess = false;

    SettingsManager* s = SettingsManager::instance();
    QString processedText = text;
    bool fmtEnabled = s->formatterEnabled() && !s->formatterCommand().isEmpty();

    if (fmtEnabled && s->formatterOrder() == SettingsManager::FormatFirst) {
        processedText = runFormatter(processedText);
    }

    if (!s->autoExternalProcessBeforeCopy()) {
        if (fmtEnabled && s->formatterOrder() == SettingsManager::ProcessFirst) {
            processedText = runFormatter(processedText);
        }
        QGuiApplication::clipboard()->setText(processedText);
        emit finished(processedText);
        return;
    }

    QString command = getFinalCommand(originalType, processedText);

    if (command.isEmpty()) {
        if (fmtEnabled && s->formatterOrder() == SettingsManager::ProcessFirst) {
            processedText = runFormatter(processedText);
        }
        QGuiApplication::clipboard()->setText(processedText);
        emit finished(processedText);
        return;
    }

    m_applyFormatterAfterProcess = (fmtEnabled && s->formatterOrder() == SettingsManager::ProcessFirst);

    executeCommand(command, processedText);
}

inline void CopyProcessor::manualProcess(const QString& text, ContentType type)
{
    m_originalTextForFallback = text;
    m_applyFormatterAfterProcess = false;

    SettingsManager* s = SettingsManager::instance();
    QString processedText = text;
    bool fmtEnabled = s->formatterEnabled() && !s->formatterCommand().isEmpty();

    if (fmtEnabled && s->formatterOrder() == SettingsManager::FormatFirst) {
        processedText = runFormatter(processedText);
    }

    QString command;
    switch(type) {
        case ContentType::Text: command = s->textProcessorCommand(); break;
        case ContentType::Formula: command = s->formulaProcessorCommand(); break;
        case ContentType::Table: command = s->tableProcessorCommand(); break;
        case ContentType::PureMath: command = s->pureMathProcessorCommand(); break;
        default: break;
    }

    if (command.isEmpty()) {
        if (fmtEnabled && s->formatterOrder() == SettingsManager::ProcessFirst) {
            processedText = runFormatter(processedText);
        }
        QGuiApplication::clipboard()->setText(processedText);
        emit finished(processedText);
        return;
    }

    m_applyFormatterAfterProcess = (fmtEnabled && s->formatterOrder() == SettingsManager::ProcessFirst);

    executeCommand(command, processedText);
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
    return ServiceUtils::isPureMathContent(text);
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
        QString escapedName = m_serviceName;
        escapedName.replace("\\", "\\\\");
        escapedName.replace("\"", "\\\"");
        fullCommand += " --mode-name \"" + escapedName + "\"";
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

    if (m_applyFormatterAfterProcess) {
        result = runFormatter(result);
        m_applyFormatterAfterProcess = false;
    }

    QGuiApplication::clipboard()->setText(result);
    emit finished(result);

    process->deleteLater();
    m_currentProcess = nullptr;
}

inline QString CopyProcessor::runFormatter(const QString& text)
{
    SettingsManager* s = SettingsManager::instance();
    QString cmd = s->formatterCommand();
    if (!s->formatterEnabled() || cmd.isEmpty()) return text;

    QProcess proc;
    proc.startCommand(cmd);
    if (!proc.waitForStarted(3000)) {
        qWarning() << "Formatter failed to start:" << cmd;
        return text;
    }
    proc.write(text.toUtf8());
    proc.closeWriteChannel();
    if (!proc.waitForFinished(10000)) {
        qWarning() << "Formatter timed out:" << cmd;
        proc.kill();
        return text;
    }

    if (proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
        QString result = QString::fromUtf8(proc.readAllStandardOutput());
        if (!result.isEmpty()) return result;
    } else {
        qWarning() << "Formatter exited with error:" << proc.readAllStandardError();
    }
    return text;
}

#endif // COPYPROCESSOR_H
