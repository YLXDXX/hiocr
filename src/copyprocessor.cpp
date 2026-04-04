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

// 核心逻辑：自动判断并执行
void CopyProcessor::processAndCopy(const QString& text, ContentType originalType)
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

// 手动处理逻辑 (不检查全局开关，直接强制执行指定类型的脚本)
void CopyProcessor::manualProcess(const QString& text, ContentType type)
{
    m_originalTextForFallback = text;
    QString command;

    // 手动模式下，直接获取该类型的脚本命令，忽略内容检测和全局开关
    SettingsManager* s = SettingsManager::instance();
    switch(type) {
        case ContentType::Text: command = s->textProcessorCommand(); break;
        case ContentType::Formula: command = s->formulaProcessorCommand(); break;
        case ContentType::Table: command = s->tableProcessorCommand(); break;
        case ContentType::PureMath: command = s->pureMathProcessorCommand(); break;
        default: break;
    }

    if (command.isEmpty()) {
        // 即使没脚本也复制原文，避免用户按快捷键没反应
        QGuiApplication::clipboard()->setText(text);
        emit finished(text);
        return;
    }

    executeCommand(command, text);
}

// 核心决策逻辑实现
QString CopyProcessor::getFinalCommand(ContentType originalType, const QString& text)
{
    SettingsManager* s = SettingsManager::instance();

    // 优先级 1: 纯数学公式检测
    // 条件：内容检测为纯数学 (isPureMathContent)
    // 注意：如果 UI 传入的是 Formula 类型，且内容是公式，我们也应该认为它是纯数学
    // 这里放宽条件：只要内容看起来是纯数学公式（无论包裹与否），就尝试使用纯数学脚本
    // 或者更简单的策略：如果 originalType 是 Formula，我们就认为它可能是纯数学。

    // 修正逻辑：检测内容格式。
    // 如果内容是纯公式结构（单块），且开启了纯数学脚本，优先使用。
    if (isPureMathContent(text)) {
        if (s->pureMathProcessorEnabled()) {
            QString cmd = s->pureMathProcessorCommand();
            if (!cmd.isEmpty()) return cmd;
        }
    }

    // 优先级 2: 根据原始识别类型匹配脚本
    // 如果是 Formula 类型但没匹配到纯数学脚本（或未开启），或者就是 Formula 类型，走这里
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
            // 混合内容通常默认作为 Text 处理，或者不做处理
            if (s->textProcessorEnabled()) {
                QString cmd = s->textProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
        case ContentType::PureMath: {
            // 如果直接显式传了 PureMath (通常不会从 UI 传，除非强制)
            if (s->pureMathProcessorEnabled()) {
                QString cmd = s->pureMathProcessorCommand();
                if (!cmd.isEmpty()) return cmd;
            }
            break;
        }
    }

    return QString();
}

bool CopyProcessor::isPureMathContent(const QString& text) const
{
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return false;

    // --- 1. 检测行间公式界定符 ---
    // 支持 $$...$$ 或 \[...\]
    bool isDisplay = (trimmed.startsWith("$$") && trimmed.endsWith("$$")) ||
    (trimmed.startsWith("\\[") && trimmed.endsWith("\\]"));

    // --- 2. 检测行内公式界定符 ---
    // 支持 $...$ (需排除 $$) 或 \(...\)
    bool isInline = (trimmed.startsWith("$") && trimmed.endsWith("$") && !trimmed.startsWith("$$")) ||
    (trimmed.startsWith("\\(") && trimmed.endsWith("\\)"));

    if (isDisplay || isInline) {
        // 简单的判断：如果中间有双换行，通常是混合内容（Markdown 段落分割）
        if (trimmed.contains("\n\n")) return false;

        // 检查是否有多个公式块拼接的情况
        // 例如：$$a$$ $$b$$，中间有空格或换行，这通常视为混合内容而非单个纯公式
        // 针对 $$ 的情况：如果数量大于2，说明有多个块
        if (trimmed.startsWith("$$") && trimmed.count("$$") > 2) return false;

        // 针对 \[ \] 的情况：如果中间再次出现界定符，视为混合
        if (trimmed.startsWith("\\[") && (trimmed.count("\\[") > 1 || trimmed.count("\\]") > 1)) return false;

        return true;
    }
    return false;
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
