// markdownbridge.h
#ifndef MARKDOWNBRIDGE_H
#define MARKDOWNBRIDGE_H

#include <QObject>

class MarkdownBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString markdown READ markdown WRITE setMarkdown NOTIFY markdownChanged)
public:
    explicit MarkdownBridge(QObject* parent = nullptr) : QObject(parent) {}

    QString markdown() const { return m_markdown; }

    void setMarkdown(const QString& markdown) {
        if (m_markdown != markdown) {
            m_markdown = markdown;
            emit markdownChanged(markdown);
        }
    }

signals:
    void markdownChanged(const QString& markdown);

private:
    QString m_markdown;
};

#endif // MARKDOWNBRIDGE_H
