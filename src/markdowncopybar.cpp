// src/markdowncopybar.cpp
#include "markdowncopybar.h"
#include "settingsmanager.h"

#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QLabel>

MarkdownCopyBar::MarkdownCopyBar(QWidget* parent)
: QWidget(parent), m_currentType(ContentType::MixedContent), m_originalRecognizeType(ContentType::Text)
{
    setupUi();
    m_processor = new CopyProcessor(this);
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

void MarkdownCopyBar::setOriginalRecognizeType(ContentType type)
{
    m_originalRecognizeType = type;
    if (m_sourceEdit) {
        updateButtonState(m_sourceEdit->toPlainText());
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

    if (isDisplayMath || isInlineMath) {
        m_currentType = ContentType::Formula;
        if (trimmed.contains("\n\n") || trimmed.count("$$") > 2) {
            m_currentType = ContentType::MixedContent;
        }
    } else {
        m_currentType = ContentType::MixedContent;
    }

    m_btnInline->setEnabled(m_currentType == ContentType::Formula);
}

void MarkdownCopyBar::onCopyOriginal()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();
    QString finalText;

    // 提取内容逻辑
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

    // 【修复】逻辑优化
    ContentType typeToUse = m_originalRecognizeType;

    // 如果当前内容结构为单公式（m_currentType == Formula），则具有“纯公式”优先级
    if (m_currentType == ContentType::Formula) {
        SettingsManager* s = SettingsManager::instance();
        // 检查是否配置了纯公式脚本
        if (s->pureMathProcessorEnabled() && !s->pureMathProcessorCommand().isEmpty()) {
            typeToUse = ContentType::PureMath;
        }
        // 如果未配置纯公式脚本，typeToUse 保持为 m_originalRecognizeType，
        // 这样会自动回退到文字/公式/表格对应的脚本，符合需求。
    }

    m_processor->processAndCopy(finalText, typeToUse);
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

    // 【修复】传递原始识别类型，而不是强制 Formula
    // CopyProcessor 会自动处理“纯公式检测”逻辑（因为这里带了 $ 包裹，会被检测为纯公式结构）
    // 如果没有纯公式脚本，会回退到原始类型脚本。
    m_processor->processAndCopy("$" + core + "$", m_originalRecognizeType);
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

    // 【修复】传递原始识别类型，而不是强制 Formula
    // 同上，CopyProcessor 会先尝试纯公式脚本（因为带了 $$ 包裹），回退时使用原始类型脚本。
    m_processor->processAndCopy(finalText, m_originalRecognizeType);
}
