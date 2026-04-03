// src/copyprocessor.cpp
#include "copyprocessor.h"
#include "settingsmanager.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>

CopyProcessor::CopyProcessor(QObject *parent)
: QObject(parent)
{
}

void CopyProcessor::processAndCopy(const QString& text, ContentType type)
{
    m_originalTextForFallback = text;
    m_currentType = type;

    // 1. 检查是否开启了自动处理
    if (!SettingsManager::instance()->autoExternalProcessBeforeCopy()) {
        // 未开启，直接复制原文
        QGuiApplication::clipboard()->setText(text);
        emit finished(text);
        return;
    }

    // 2. 根据类型获取命令 (逻辑优化：TODO 准备)
    QString command;

    // 【预留 TODO 逻辑】：如果检测为纯数学公式，且设置了纯数学脚本，则使用
    // if (type == ContentType::PureMath && !SettingsManager::instance()->pureMathProcessorCommand().isEmpty()) {
    //     command = SettingsManager::instance()->pureMathProcessorCommand();
    // }

    // 当前逻辑：根据类型选择脚本
    command = SettingsManager::instance()->externalProcessorCommand();

    if (command.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
        emit finished(text);
        return;
    }

    executeCommand(command, text);
}

void CopyProcessor::executeCommand(const QString& command, const QString& text)
{
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess->deleteLater();
    }

    m_currentProcess = new QProcess(this);
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CopyProcessor::onProcessFinished);

    qDebug() << "Executing copy processor:" << command;
    m_currentProcess->startCommand(command);
    m_currentProcess->write(text.toUtf8());
    m_currentProcess->closeWriteChannel();
}

void CopyProcessor::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString result;

    if (status == QProcess::NormalExit && exitCode == 0) {
        result = QString::fromUtf8(process->readAllStandardOutput());
        if (result.isEmpty()) result = m_originalTextForFallback; // 脚本无输出，回退
    } else {
        QString err = process->readAllStandardError();
        qWarning() << "Copy processor failed:" << err;
        result = m_originalTextForFallback; // 失败时回退原文
        emit error("External processor failed: " + err);
    }

    QGuiApplication::clipboard()->setText(result);
    emit finished(result);

    process->deleteLater();
    m_currentProcess = nullptr;
}
