#include "markdownsourceeditor.h"
#include "settingsmanager.h"

#include <QFont>

MarkdownSourceEditor::MarkdownSourceEditor(QWidget* parent)
: QPlainTextEdit(parent)
{
    QFont font("Courier New", SettingsManager::instance()->sourceEditorFontSize());
    setFont(font);

    // 【新增】监听设置变化
    connect(SettingsManager::instance(), &SettingsManager::sourceEditorFontSizeChanged, this, &MarkdownSourceEditor::onFontSizeChanged);
}

void MarkdownSourceEditor::onFontSizeChanged(int size)
{
    QFont currentFont = font();
    currentFont.setPointSize(size);
    setFont(currentFont);
}
