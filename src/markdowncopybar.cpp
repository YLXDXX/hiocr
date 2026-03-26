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
    layout->setSpacing(4); // 控件间距，与 PromptBar 保持一致

    // 1. 标签固定在最左侧
    layout->addWidget(new QLabel("复制选项:", this));

    // 2. 创建按钮
    m_btnOriginal = new QPushButton("内容", this);
    m_btnInline = new QPushButton("行内", this);
    m_btnDisplay = new QPushButton("行间", this);

    // 3. 设置按钮宽度
    // 文字为2个汉字，约等于30-40px。
    // 设置为 60px 可以确保按钮比文字宽，方便点击，且视觉上统一
    int btnWidth = 60;
    m_btnOriginal->setFixedWidth(btnWidth);
    m_btnInline->setFixedWidth(btnWidth);
    m_btnDisplay->setFixedWidth(btnWidth);

    // 连接信号槽
    connect(m_btnOriginal, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyOriginal);
    connect(m_btnInline, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyInline);
    connect(m_btnDisplay, &QPushButton::clicked, this, &MarkdownCopyBar::onCopyDisplay);

    // 4. 添加按钮到布局
    layout->addWidget(m_btnOriginal);
    layout->addWidget(m_btnInline);
    layout->addWidget(m_btnDisplay);

    // 5. 添加弹簧，将左侧内容和按钮推向左边，右侧留空
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

    if (isDisplayMath || isInlineMath)
    {
        m_currentType = SingleFormula;
        if (trimmed.contains("\n\n") || trimmed.count("$$") > 2)
        {
            m_currentType = MixedContent;
        }
    }
    else
    {
        m_currentType = MixedContent;
    }

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

// 【新增】核心复制逻辑，处理外部程序调用
void MarkdownCopyBar::executeCopy(const QString& text)
{
    // 检查是否需要自动调用外部程序
    if (SettingsManager::instance()->autoExternalProcessBeforeCopy()) {
        QString command = SettingsManager::instance()->externalProcessorCommand();

        if (!command.isEmpty()) {
            qDebug() << "Auto external process triggered for copy.";

            // 禁用按钮防止重复点击
            m_btnOriginal->setEnabled(false);
            m_btnInline->setEnabled(false);
            m_btnDisplay->setEnabled(false);

            if (!m_externalProcess) {
                m_externalProcess = new QProcess(this);
                connect(m_externalProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, &MarkdownCopyBar::onExternalProcessFinished);
            }

            m_externalProcess->startCommand(command);
            m_externalProcess->write(text.toUtf8());
            m_externalProcess->closeWriteChannel();
            return; // 异步等待，不直接复制
        }
    }

    // 直接复制
    QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    qDebug() << "Copied directly:" << text.left(20);
}

// 【新增】外部程序处理完成回调
void MarkdownCopyBar::onExternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // 恢复按钮状态
    m_btnOriginal->setEnabled(true);
    m_btnInline->setEnabled(true);
    // m_btnDisplay 的状态需要根据当前类型重新计算，这里简单起见直接启用
    m_btnDisplay->setEnabled(true);

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString result = QString::fromUtf8(m_externalProcess->readAllStandardOutput());
        QApplication::clipboard()->setText(result, QClipboard::Clipboard);
        qDebug() << "Copied after external process:" << result.left(20);
    } else {
        QString error = m_externalProcess->readAllStandardError();
        qWarning() << "External process failed:" << error;
        // 即使失败，也将原始内容复制（可选行为，这里选择提示用户）
        // QApplication::clipboard()->setText(m_sourceEdit->toPlainText());
    }
}

void MarkdownCopyBar::onCopyOriginal()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();
    QString finalText;

    // 【修改】需求2：仅复制纯公式内容
    if (m_currentType == SingleFormula) {
        // 去除所有可能的环境包裹
        if (content.startsWith("$$") && content.endsWith("$$")) {
            finalText = content.mid(2, content.length() - 4).trimmed();
        } else if (content.startsWith("$") && content.endsWith("$")) {
            finalText = content.mid(1, content.length() - 2).trimmed();
        } else {
            finalText = content;
        }
    } else {
        // 混合内容保持原样
        finalText = content;
    }

    executeCopy(finalText);
}

void MarkdownCopyBar::onCopyInline()
{
    if (!m_sourceEdit) return;
    QString content = m_sourceEdit->toPlainText().trimmed();

    if (m_currentType != SingleFormula) return;

    QString core;
    if (content.startsWith("$$") && content.endsWith("$$")) {
        core = content.mid(2, content.length() - 4).trimmed();
    } else if (content.startsWith("$") && content.endsWith("$")) {
        core = content.mid(1, content.length() - 2).trimmed();
    } else {
        core = content;
    }

    QString finalText = "$" + core + "$";
    executeCopy(finalText);
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

    executeCopy(finalText);
}
