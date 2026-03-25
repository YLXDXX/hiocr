// markdownbridge.h
#ifndef MARKDOWNBRIDGE_H
#define MARKDOWNBRIDGE_H

#include <QObject>

class MarkdownBridge : public QObject {
    Q_OBJECT
    // 添加 mathFont 属性，使其在 JS 中可访问
    Q_PROPERTY(QString markdown READ markdown WRITE setMarkdown NOTIFY markdownChanged)
    Q_PROPERTY(QString mathFont READ mathFont WRITE setMathFont NOTIFY mathFontChanged)

public:
    explicit MarkdownBridge(QObject* parent = nullptr) : QObject(parent) {}

    QString markdown() const { return m_markdown; }
    void setMarkdown(const QString& markdown) {
        if (m_markdown != markdown) {
            m_markdown = markdown;
            emit markdownChanged(markdown);
        }
    }

    // 【新增】
    QString mathFont() const { return m_mathFont; }
    void setMathFont(const QString& font) {
        if (m_mathFont != font) {
            m_mathFont = font;
            emit mathFontChanged(font);
        }
    }

signals:
    void markdownChanged(const QString& markdown);
    void mathFontChanged(const QString& font); // 【新增】

private:
    QString m_markdown;
    QString m_mathFont; // 【新增】
};

#endif // MARKDOWNBRIDGE_H
