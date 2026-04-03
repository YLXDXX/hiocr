// src/markdowncopybar.cpp
#include "markdowncopybar.h"
#include "settingsmanager.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QDebug>
#include <QPlainTextEdit>

MarkdownCopyBar::MarkdownCopyBar(QWidget* parent)
: QWidget(parent), m_currentType(ContentType::MixedContent)
{
    setupUi();
    m_processor = new CopyProcessor(this); // 初始化处理器
}

void MarkdownCopyBar::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);
    layout->setSpacing(4);

    layout->addWidget(new QLabel("复制选项:", this));

    m_btnOriginal = new QPushButton("内容", this);
    m_btnInline = new QPushButton("行内", this);
    m_btnDisplay = new QPushButton("行间", this);

    int btnWidth = 60;
    m_btnOriginal->setFixedWidth(btnWidth);
    m_btnInline->setFixedWidth(btnWidth);
    m_btnDisplay->setFixedWidth(btnWidth);

    connect(m_btnOriginal, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyOriginal);
    connect(m_btnInline, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyInline);
    connect(m_btnDisplay, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyDisplay);

    layout->addWidget(m_btnOriginal);
    layout->addWidget(m_btnInline);
    layout->addWidget(m_btnDisplay);
    layout->addStretch();
}

void MarkdownCopyBar::setSourceEditor(QPlainTextEdit* editor)
{
    m_sourceEdit = editor;
    if (m_sourceEdit) {
        connect(m_sourceEdit, &QPlainTextEdit::textChanged, this, &MarkdownCopyBar::onSourceTextChanged);
    }
}

void MarkdownCopyBar::onSourceTextChanged()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText();
    updateButtonState(content);
}

void MarkdownCopyBar::updateButtonState(const QString& content)
{
    QString trimmed = content.trimmed();
    bool isDisplayMath = trimmed.startsWith("$$") && trimmed.endsWith("$$");
    bool isInlineMath = (trimmed.startsWith("$") && trimmed.endsWith("$")) && !isDisplayMath;

    // 【优化】类型判断逻辑，为 TODO 纯数学公式做准备
    if (isDisplayMath || isInlineMath) {
        // 当前假设单公式场景
        // TODO: 未来这里可以进一步分析是否是纯数学公式（通过简单正则或调用外部检查器）
        m_currentType = ContentType::Formula;

        if (trimmed.contains("\n\n") || trimmed.count("$$") > 2) {
            m_currentType = ContentType::MixedContent;
        }
    } else {
        m_currentType = ContentType::MixedContent;
    }

    if (m_currentType == ContentType::Formula) {
        m_btnInline->setEnabled(true);
        m_btnInline->setToolTip("复制为 $...$ 格式");
    } else {
        m_btnInline->setEnabled(false);
        m_btnInline->setToolTip("混合内容不支持统一转为行内公式");
    }
}

// 统一调用 Processor
void MarkdownCopyBar::executeCopy(const QString& text)
{
    m_processor->processAndCopy(text, m_currentType);
}

void MarkdownCopyBar::onCopyOriginal()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();
    QString finalText;

    if (m_currentType == ContentType::Formula) {
        if (content.startsWith("$$") && content.endsWith("$$")) {
            finalText = content.mid(2, content.length() - 4).trimmed();
        } else if (content.startsWith("$") && content.endsWith("$")) {
            finalText = content.mid(1, content.length() - 2).trimmed();
        } else {
            finalText = content;
        }
    } else {
        finalText = content;
    }
    executeCopy(finalText);
}

void MarkdownCopyBar::onCopyInline()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();
    if (m_currentType != ContentType::Formula) return;

    QString core;
    if (content.startsWith("$$") && content.endsWith("$$")) {
        core = content.mid(2, content.length() - 4).trimmed();
    } else if (content.startsWith("$") && content.endsWith("$")) {
        core = content.mid(1, content.length() - 2).trimmed();
    } else {
        core = content;
    }
    executeCopy("$" + core + "$");
}

void MarkdownCopyBar::onCopyDisplay()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText();
    QString finalText;
    QString env = SettingsManager::instance()->displayMathEnvironment();

    auto extractContent = [](const QString& raw) -> QString {
        QString t = raw.trimmed();
        if (t.startsWith("$$") && t.endsWith("$$")) return t.mid(2, t.length() - 4).trimmed();
        if (t.startsWith("$") && t.endsWith("$")) return t.mid(1, t.length() - 2).trimmed();
        return t;
    };

    if (m_currentType == ContentType::Formula) {
        QString core = extractContent(content);
        if (env == "$$") {
            finalText = "$$\n" + core + "\n$$";
        } else {
            finalText = "\\begin{" + env + "}\n\t" + core + "\n\\end{" + env + "}";
        }
    } else {
        finalText = content;
        QRegularExpression re("\\$\\$([\\s\\S]*?)\\$\\$");
        QRegularExpressionMatchIterator it = re.globalMatch(finalText);
        QString resultText;
        int lastEnd = 0;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            resultText += finalText.mid(lastEnd, match.capturedStart() - lastEnd);
            QString mathCore = match.captured(1).trimmed();

            if (env == "$$") {
                resultText += "$$\n" + mathCore + "\n$$";
            } else {
                resultText += "\\begin{" + env + "}\n\t" + mathCore + "\n\\end{" + env + "}";
            }
            lastEnd = match.capturedEnd();
        }
        resultText += finalText.mid(lastEnd);
        finalText = resultText;
    }

    executeCopy(finalText);
}
