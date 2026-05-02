#ifndef MARKDOWNSOURCEEDITOR_H
#define MARKDOWNSOURCEEDITOR_H

#include <QPlainTextEdit>
#include <QFont>
#include <QPainter>
#include <QTextBlock>
#include <QWidget>

#include "settingsmanager.h"

class MarkdownSourceEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownSourceEditor(QWidget* parent = nullptr);

    void setShowLineNumbers(bool show);
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent* event);

private slots:
    void onFontSizeChanged(int size);
    void onShowLineNumbersChanged(bool show);

private:
    void updateLineNumberAreaWidth(int blockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void resizeEvent(QResizeEvent* event) override;

    QWidget* m_lineNumberArea;
    bool m_showLineNumbers;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(MarkdownSourceEditor* editor) : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    MarkdownSourceEditor* m_editor;
};

// --- Implementation ---

inline MarkdownSourceEditor::MarkdownSourceEditor(QWidget* parent)
: QPlainTextEdit(parent)
{
    QFont font("Courier New", SettingsManager::instance()->sourceEditorFontSize());
    setFont(font);

    connect(SettingsManager::instance(), &SettingsManager::sourceEditorFontSizeChanged, this, &MarkdownSourceEditor::onFontSizeChanged);
    connect(SettingsManager::instance(), &SettingsManager::showLineNumbersChanged, this, &MarkdownSourceEditor::onShowLineNumbersChanged);

    m_lineNumberArea = new LineNumberArea(this);
    m_showLineNumbers = SettingsManager::instance()->showLineNumbers();
    m_lineNumberArea->setVisible(m_showLineNumbers);

    connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int newBlockCount){
        if (m_showLineNumbers) {
            updateLineNumberAreaWidth(newBlockCount);
        }
    });
    connect(this, &QPlainTextEdit::updateRequest, this, [this](const QRect& rect, int dy){
        if (m_showLineNumbers) {
            if (dy)
                m_lineNumberArea->scroll(0, dy);
            else
                m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
        }
        if (rect.contains(viewport()->rect()))
            updateLineNumberAreaWidth(blockCount());
    });

    if (m_showLineNumbers) {
        updateLineNumberAreaWidth(blockCount());
    }
}

inline void MarkdownSourceEditor::setShowLineNumbers(bool show)
{
    m_showLineNumbers = show;
    m_lineNumberArea->setVisible(show);
    if (show) {
        updateLineNumberAreaWidth(blockCount());
    } else {
        setViewportMargins(0, 0, 0, 0);
    }
}

inline void MarkdownSourceEditor::onFontSizeChanged(int size)
{
    QFont currentFont = font();
    currentFont.setPointSize(size);
    setFont(currentFont);
}

inline void MarkdownSourceEditor::onShowLineNumbersChanged(bool show)
{
    setShowLineNumbers(show);
}

inline int MarkdownSourceEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

inline void MarkdownSourceEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

inline void MarkdownSourceEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
}

inline void MarkdownSourceEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

inline void MarkdownSourceEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(160, 160, 160));
            painter.drawText(0, top, m_lineNumberArea->width() - 4, fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

#endif // MARKDOWNSOURCEEDITOR_H
