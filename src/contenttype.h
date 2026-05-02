// src/contenttype.h
#ifndef CONTENTTYPE_H
#define CONTENTTYPE_H

#include <QString>

enum class ContentType {
    Text,
    Formula,
    Table,
    MixedContent,
    PureMath
};

namespace ServiceUtils {
    inline bool isChandraService(const QString& serviceName) {
        return serviceName.contains("chandra", Qt::CaseInsensitive);
    }

    inline bool isPureMathContent(const QString& text) {
        QString trimmed = text.trimmed();
        if (trimmed.isEmpty()) return false;

        if (trimmed.contains("<math")) return false;

        bool isDisplay = (trimmed.startsWith("$$") && trimmed.endsWith("$$")) ||
        (trimmed.startsWith("\\[") && trimmed.endsWith("\\]"));

        bool isInline = (trimmed.startsWith("$") && trimmed.endsWith("$") && !trimmed.startsWith("$$")) ||
        (trimmed.startsWith("\\(") && trimmed.endsWith("\\)"));

        if (isDisplay || isInline) {
            if (trimmed.contains("\n\n")) return false;
            if (trimmed.startsWith("$$") && trimmed.count("$$") > 2) return false;
            if (trimmed.startsWith("\\[") && (trimmed.count("\\[") > 1 || trimmed.count("\\]") > 1)) return false;
            return true;
        }
        return false;
    }
}

#endif // CONTENTTYPE_H
