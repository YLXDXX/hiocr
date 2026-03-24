#include "markdowncopybar.h"
#include "settingsmanager.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QDebug>
#include <QPlainTextEdit>
#include <QString>

MarkdownCopyBar::MarkdownCopyBar(QWidget* parent)
: QWidget(parent), m_currentType(MixedContent)
{
    setupUi();
}

void MarkdownCopyBar::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);

    layout->addWidget(new QLabel("复制选项:", this));

    m_btnOriginal = new QPushButton("原样复制", this);
    m_btnInline = new QPushButton("行内公式", this);
    m_btnDisplay = new QPushButton("行间公式", this);

    connect(m_btnOriginal, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyOriginal);
    connect(m_btnInline, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyInline);
    connect(m_btnDisplay, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyDisplay);

    layout->addWidget(m_btnOriginal);
    layout->addWidget(m_btnInline);
    layout->addWidget(m_btnDisplay);
    layout->addStretch();
}

// 【新增】实现设置编辑器指针
void MarkdownCopyBar::setSourceEditor(QPlainTextEdit* editor)
{
    m_sourceEdit = editor;
    if (m_sourceEdit) {
        // 连接文本变化信号，实时更新按钮状态
        connect(m_sourceEdit, &QPlainTextEdit::textChanged, this, &MarkdownCopyBar::onSourceTextChanged);
    }
}

// 【新增】文本变化时自动判断类型
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

    if (isDisplayMath || isInlineMath)
    {
        m_currentType = SingleFormula;
        // 如果包含多个公式块或双换行，则视为混合
        if (trimmed.contains("\n\n") || trimmed.count("$$") > 2)
        {
            m_currentType = MixedContent;
        }
    }
    else
    {
        m_currentType = MixedContent;
    }

    // 更新 UI
    if (m_currentType == SingleFormula)
    {
        m_btnInline->setEnabled(true);
        m_btnInline->setToolTip("复制为 $...$ 格式");
        m_btnDisplay->setToolTip("使用设置的行间环境包裹");
    }
    else
    {
        m_btnInline->setEnabled(false);
        m_btnInline->setToolTip("混合内容不支持统一转为行内公式");
        m_btnDisplay->setToolTip("将源码中所有 $$ 替换为设定的行间环境");
    }
}

void MarkdownCopyBar::onCopyOriginal()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText();

    QClipboard* cb = QApplication::clipboard();
    cb->setText(content, QClipboard::Clipboard);

    qDebug() << "Copied Original:" << content.left(20);
}

void MarkdownCopyBar::onCopyInline()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();

    if (m_currentType != SingleFormula) return;

    // 去除外层
    if (content.startsWith("$$") && content.endsWith("$$"))
    {
        content = content.mid(2, content.length() - 4).trimmed();
    }
    else if (content.startsWith("$") && content.endsWith("$"))
    {
        content = content.mid(1, content.length() - 2).trimmed();
    }

    QString finalText = "$" + content + "$";

    QClipboard* cb = QApplication::clipboard();
    cb->setText(finalText, QClipboard::Clipboard);
    qDebug() << "Copied Inline:" << finalText;
}

void MarkdownCopyBar::onCopyDisplay()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText();
    QString finalText;
    QString env = SettingsManager::instance()->displayMathEnvironment();

    // 辅助函数：提取核心内容
    auto extractContent = [](const QString& raw) -> QString {
        QString t = raw.trimmed();
        if (t.startsWith("$$") && t.endsWith("$$")) return t.mid(2, t.length() - 4).trimmed();
        if (t.startsWith("$") && t.endsWith("$")) return t.mid(1, t.length() - 2).trimmed();
        return t;
    };

    if (m_currentType == SingleFormula)
    {
        QString core = extractContent(content);
        if (env == "$$")
        {
            finalText = "$$\n" + core + "\n$$";
        }
        else
        {
            finalText = "\\begin{" + env + "}\n\t" + core + "\n\\end{" + env + "}";
        }
    }
    else
    {
        finalText = content;
        QRegularExpression re("\\$\\$([\\s\\S]*?)\\$\\$");
        QRegularExpressionMatchIterator it = re.globalMatch(finalText);
        QString resultText;
        int lastEnd = 0;

        while (it.hasNext())
        {
            QRegularExpressionMatch match = it.next();
            resultText += finalText.mid(lastEnd, match.capturedStart() - lastEnd);

            QString mathCore = match.captured(1).trimmed();

            if (env == "$$")
            {
                resultText += "$$\n" + mathCore + "\n$$";
            }
            else
            {
                resultText += "\\begin{" + env + "}\n\t" + mathCore + "\n\\end{" + env + "}";
            }
            lastEnd = match.capturedEnd();
        }
        resultText += finalText.mid(lastEnd);
        finalText = resultText;
    }

    QClipboard* cb = QApplication::clipboard();
    cb->setText(finalText, QClipboard::Clipboard);
    qDebug() << "Copied Display:" << finalText.left(50);
}
