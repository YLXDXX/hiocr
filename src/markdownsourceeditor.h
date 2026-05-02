#ifndef MARKDOWNSOURCEEDITOR_H
#define MARKDOWNSOURCEEDITOR_H

#include <QPlainTextEdit>
#include <QFont>

#include "settingsmanager.h"

class MarkdownSourceEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownSourceEditor(QWidget* parent = nullptr);

private slots:
    void onFontSizeChanged(int size);
};

// --- Implementation ---

inline MarkdownSourceEditor::MarkdownSourceEditor(QWidget* parent)
: QPlainTextEdit(parent)
{
    QFont font("Courier New", SettingsManager::instance()->sourceEditorFontSize());
    setFont(font);

    connect(SettingsManager::instance(), &SettingsManager::sourceEditorFontSizeChanged, this, &MarkdownSourceEditor::onFontSizeChanged);
}

inline void MarkdownSourceEditor::onFontSizeChanged(int size)
{
    QFont currentFont = font();
    currentFont.setPointSize(size);
    setFont(currentFont);
}

#endif // MARKDOWNSOURCEEDITOR_H
