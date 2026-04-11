// markdownbridge.h
#ifndef MARKDOWNBRIDGE_H
#define MARKDOWNBRIDGE_H

#include <QObject>

class MarkdownBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString markdown READ markdown WRITE setMarkdown NOTIFY markdownChanged)
    Q_PROPERTY(QString mathFont READ mathFont WRITE setMathFont NOTIFY mathFontChanged)
    Q_PROPERTY(QString serviceName READ serviceName WRITE setServiceName NOTIFY serviceNameChanged)

public:
    explicit MarkdownBridge(QObject* parent = nullptr) : QObject(parent) {}

    QString markdown() const { return m_markdown; }
    void setMarkdown(const QString& markdown) {
        if (m_markdown != markdown) {
            m_markdown = markdown;
            emit markdownChanged(markdown);
        }
    }

    QString mathFont() const { return m_mathFont; }
    void setMathFont(const QString& font) {
        if (m_mathFont != font) {
            m_mathFont = font;
            emit mathFontChanged(font);
        }
    }

    // 【新增】当前服务名称，用于 JS 判断渲染模式
    QString serviceName() const { return m_serviceName; }
    void setServiceName(const QString& name) {
        if (m_serviceName != name) {
            m_serviceName = name;
            emit serviceNameChanged(name);
        }
    }

signals:
    void markdownChanged(const QString& markdown);
    void mathFontChanged(const QString& font);
    void serviceNameChanged(const QString& name); // 【新增】

private:
    QString m_markdown;
    QString m_mathFont;
    QString m_serviceName; // 【新增】
};

#endif // MARKDOWNBRIDGE_H
