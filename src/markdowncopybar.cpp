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

// 【新增】
void MarkdownCopyBar::setCurrentServiceName(const QString& name)
{
    m_currentServiceName = name;
    // 同步到 CopyProcessor
    if (m_processor) {
        m_processor->setServiceName(name);
    }
    // 更新按钮状态
    if (m_sourceEdit) {
        updateButtonState(m_sourceEdit->toPlainText());
    }
}

// 【新增】
bool MarkdownCopyBar::isChandraService() const
{
    return m_currentServiceName.contains("chandra", Qt::CaseInsensitive);
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

    // 【修改】Chandra 模式下，禁用行内/行间按钮
    if (isChandraService() && trimmed.contains("<math")) {
        m_currentType = ContentType::MixedContent;
        m_btnInline->setEnabled(false);
        m_btnDisplay->setEnabled(false);
        return;
    }

    bool isDisplay = (trimmed.startsWith("$$") && trimmed.endsWith("$$")) ||
    (trimmed.startsWith("\\[") && trimmed.endsWith("\\]"));

    bool isInline = (trimmed.startsWith("$") && trimmed.endsWith("$") && !trimmed.startsWith("$$")) ||
    (trimmed.startsWith("\\(") && trimmed.endsWith("\\)"));

    if (isDisplay || isInline) {
        m_currentType = ContentType::Formula;
        if (trimmed.contains("\n\n")) {
            m_currentType = ContentType::MixedContent;
        }
        if (trimmed.startsWith("$$") && trimmed.count("$$") > 2) {
            m_currentType = ContentType::MixedContent;
        }
    } else {
        m_currentType = ContentType::MixedContent;
    }

    m_btnInline->setEnabled(m_currentType == ContentType::Formula);
    m_btnDisplay->setEnabled(true);
}

void MarkdownCopyBar::onCopyOriginal()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();

    // 【修改】Chandra 模式下，直接将原始内容通过 CopyProcessor 处理
    // CopyProcessor 会自动附加 --mode-name 参数
    if (isChandraService() && content.contains("<math")) {
        m_processor->processAndCopy(content, m_originalRecognizeType);
        return;
    }

    QString finalText;

    if (m_currentType == ContentType::Formula) {
        if (content.startsWith("$$") && content.endsWith("$$")) {
            finalText = content.mid(2, content.length() - 4).trimmed();
        } else if (content.startsWith("\\[") && content.endsWith("\\]")) {
            finalText = content.mid(2, content.length() - 4).trimmed();
        } else if (content.startsWith("\\(") && content.endsWith("\\)")) {
            finalText = content.mid(2, content.length() - 4).trimmed();
        } else if (content.startsWith("$") && content.endsWith("$")) {
            finalText = content.mid(1, content.length() - 2).trimmed();
        } else {
            finalText = content;
        }
    } else {
        finalText = content;
    }

    ContentType typeToUse = m_originalRecognizeType;
    if (m_currentType == ContentType::Formula) {
        SettingsManager* s = SettingsManager::instance();
        if (s->pureMathProcessorEnabled() && !s->pureMathProcessorCommand().isEmpty()) {
            typeToUse = ContentType::PureMath;
        }
    }
    m_processor->processAndCopy(finalText, typeToUse);
}

void MarkdownCopyBar::onCopyInline()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();

    // 【修改】Chandra 模式下，走通用处理
    if (isChandraService() && content.contains("<math")) {
        m_processor->processAndCopy(content, m_originalRecognizeType);
        return;
    }

    if (m_currentType != ContentType::Formula) return;

    QString core;

    if (content.startsWith("$$") && content.endsWith("$$")) {
        core = content.mid(2, content.length() - 4).trimmed();
    } else if (content.startsWith("\\[") && content.endsWith("\\]")) {
        core = content.mid(2, content.length() - 4).trimmed();
    } else if (content.startsWith("\\(") && content.endsWith("\\)")) {
        core = content.mid(2, content.length() - 4).trimmed();
    } else if (content.startsWith("$") && content.endsWith("$")) {
        core = content.mid(1, content.length() - 2).trimmed();
    } else {
        core = content;
    }

    m_processor->processAndCopy("$" + core + "$", m_originalRecognizeType);
}

void MarkdownCopyBar::onCopyDisplay()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText();

    // 【修改】Chandra 模式下，走通用处理
    if (isChandraService() && content.contains("<math")) {
        m_processor->processAndCopy(content, m_originalRecognizeType);
        return;
    }

    QString finalText;
    QString env = SettingsManager::instance()->displayMathEnvironment();

    auto extractContent = [](const QString& raw) -> QString {
        QString t = raw.trimmed();
        if (t.startsWith("$$") && t.endsWith("$$")) return t.mid(2, t.length() - 4).trimmed();
        if (t.startsWith("\\[") && t.endsWith("\\]")) return t.mid(2, t.length() - 4).trimmed();
        if (t.startsWith("\\(") && t.endsWith("\\)")) return t.mid(2, t.length() - 4).trimmed();
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

    m_processor->processAndCopy(finalText, m_originalRecognizeType);
}
