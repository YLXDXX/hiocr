#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QImage>
#include <QSqlDatabase>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include "copyprocessor.h"

#include "settingsmanager.h"

struct HistoryRecord {
    int id = -1;
    QString imageHash;
    QString cachedImagePath;
    QString resultText;
    ContentType recognitionType;
    QDateTime timestamp;
};

class HistoryManager : public QObject
{
    Q_OBJECT
public:
    static HistoryManager* instance();

    void init();

    void saveRecord(const QImage& image, const QString& result, ContentType type);

    QList<HistoryRecord> getRecentRecords(int limit = -1, int offset = 0);

    int getTotalCount();

    void deleteRecord(int id);

    void clearAll();

    HistoryRecord getRecordById(int id);

private:
    explicit HistoryManager(QObject *parent = nullptr);
    bool ensureTableExists();
    QString hashImage(const QImage& image) const;
    QString saveImageCache(const QImage& image, const QString& hash);

    QSqlDatabase m_db;
    QString m_dataPath;
};

// --- Implementation ---

inline HistoryManager* HistoryManager::instance()
{
    static HistoryManager* s_instance = nullptr;
    if (!s_instance) s_instance = new HistoryManager();
    return s_instance;
}

inline HistoryManager::HistoryManager(QObject *parent) : QObject(parent)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) dir.mkpath(".");

    m_dataPath = configPath;
}

inline void HistoryManager::init()
{
    QString dbPath = m_dataPath + "/history.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE", "history_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open history database:" << m_db.lastError().text();
        return;
    }

    ensureTableExists();

    QDir imgDir(m_dataPath + "/history_images");
    if (!imgDir.exists()) imgDir.mkpath(".");
}

inline bool HistoryManager::ensureTableExists()
{
    QSqlQuery query(m_db);
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "image_hash TEXT UNIQUE, "
        "cached_image_path TEXT, "
        "result_text TEXT, "
        "recognition_type INTEGER, "
        "timestamp DATETIME DEFAULT (datetime('now', 'localtime'))"
        ")"
    );
    if (!success) {
        qWarning() << "Failed to create table:" << query.lastError().text();
    }
    return success;
}

inline QString HistoryManager::hashImage(const QImage& image) const
{
    if (image.isNull()) return QString();

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return QCryptographicHash::hash(byteArray, QCryptographicHash::Md5).toHex();
}

inline QString HistoryManager::saveImageCache(const QImage& image, const QString& hash)
{
    QString fileName = m_dataPath + "/history_images/" + hash + ".png";

    if (QFile::exists(fileName)) return fileName;

    if (image.save(fileName, "PNG")) {
        return fileName;
    }
    return QString();
}

inline void HistoryManager::saveRecord(const QImage& image, const QString& result, ContentType type)
{
    if (image.isNull() || result.isEmpty()) return;

    if (!SettingsManager::instance()->saveHistoryEnabled()) return;

    QString hash = hashImage(image);
    QString cachedPath = saveImageCache(image, hash);

    QSqlQuery query(m_db);

    query.prepare(
        "INSERT OR REPLACE INTO history (image_hash, cached_image_path, result_text, recognition_type, timestamp) "
        "VALUES (:hash, :path, :text, :type, datetime('now', 'localtime'))"
    );

    query.bindValue(":hash", hash);
    query.bindValue(":path", cachedPath);
    query.bindValue(":text", result);
    query.bindValue(":type", static_cast<int>(type));

    if (!query.exec()) {
        qWarning() << "Failed to save history record:" << query.lastError().text();
        return;
    }

    int limit = SettingsManager::instance()->historyLimit();
    if (limit > 0) {
        QSqlQuery countQuery(m_db);
        if (!countQuery.exec("SELECT COUNT(*) FROM history")) {
            qWarning() << "Failed to count history:" << countQuery.lastError().text();
            return;
        }
        if (countQuery.next() && countQuery.value(0).toInt() > limit) {
            int deleteCount = countQuery.value(0).toInt() - limit;

            QSqlQuery findOldQuery(m_db);
            findOldQuery.prepare("SELECT id, cached_image_path FROM history ORDER BY timestamp ASC LIMIT ?");
            findOldQuery.addBindValue(deleteCount);

            if (findOldQuery.exec()) {
                QList<int> ids;
                QStringList paths;
                while (findOldQuery.next()) {
                    ids << findOldQuery.value(0).toInt();
                    paths << findOldQuery.value(1).toString();
                }

                if (!ids.isEmpty()) {
                    QSqlQuery deleteQuery(m_db);
                    QString placeholders;
                    for (int i = 0; i < ids.size(); ++i) {
                        if (i > 0) placeholders += ",";
                        placeholders += QString(":id%1").arg(i);
                    }
                    deleteQuery.prepare(QString("DELETE FROM history WHERE id IN (%1)").arg(placeholders));
                    for (int i = 0; i < ids.size(); ++i) {
                        deleteQuery.bindValue(QString(":id%1").arg(i), ids[i]);
                    }

                    if (!deleteQuery.exec()) {
                        qWarning() << "Failed to delete old history records:" << deleteQuery.lastError().text();
                    }

                    for (const QString& path : paths) {
                        if (!path.isEmpty()) {
                            QFile::remove(path);
                        }
                    }
                    qDebug() << "History cleanup: removed" << deleteCount << "old records.";
                }
            } else {
                qWarning() << "Failed to find old records for cleanup:" << findOldQuery.lastError().text();
            }
        }
    }
}

inline QList<HistoryRecord> HistoryManager::getRecentRecords(int limit, int offset)
{
    if (limit < 0) {
        int configuredLimit = SettingsManager::instance()->historyLimit();
        limit = (configuredLimit == 0) ? 999999 : configuredLimit;
    }

    QList<HistoryRecord> list;
    QSqlQuery query(m_db);

    query.prepare("SELECT id, cached_image_path, result_text, recognition_type, timestamp "
    "FROM history ORDER BY timestamp DESC LIMIT ? OFFSET ?");
    query.addBindValue(limit);
    query.addBindValue(offset);

    if (query.exec()) {
        while (query.next()) {
            HistoryRecord record;
            record.id = query.value(0).toInt();
            record.cachedImagePath = query.value(1).toString();
            record.resultText = query.value(2).toString();
            record.recognitionType = static_cast<ContentType>(query.value(3).toInt());
            record.timestamp = QDateTime::fromString(query.value(4).toString(), "yyyy-MM-dd hh:mm:ss");
            list.append(record);
        }
    } else {
        qWarning() << "Failed to query recent records:" << query.lastError().text();
    }
    return list;
}

inline int HistoryManager::getTotalCount()
{
    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM history") && query.next()) {
        return query.value(0).toInt();
    }
    qWarning() << "Failed to get total count:" << query.lastError().text();
    return 0;
}

inline HistoryRecord HistoryManager::getRecordById(int id)
{
    HistoryRecord record;
    QSqlQuery query(m_db);
    query.prepare("SELECT image_hash, cached_image_path, result_text, recognition_type, timestamp FROM history WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        record.id = id;
        record.imageHash = query.value(0).toString();
        record.cachedImagePath = query.value(1).toString();
        record.resultText = query.value(2).toString();
        record.recognitionType = static_cast<ContentType>(query.value(3).toInt());
        record.timestamp = QDateTime::fromString(query.value(4).toString(), "yyyy-MM-dd hh:mm:ss");
    } else {
        if (!query.exec()) {
            qWarning() << "Failed to get record by id:" << query.lastError().text();
        }
    }
    return record;
}

inline void HistoryManager::deleteRecord(int id)
{
    QSqlQuery findQuery(m_db);
    findQuery.prepare("SELECT cached_image_path FROM history WHERE id = ?");
    findQuery.addBindValue(id);

    QString imagePath;
    if (findQuery.exec() && findQuery.next()) {
        imagePath = findQuery.value(0).toString();
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM history WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        if (!imagePath.isEmpty()) {
            QFile::remove(imagePath);
        }
    } else {
        qWarning() << "Failed to delete record:" << query.lastError().text();
    }
}

inline void HistoryManager::clearAll()
{
    QSqlQuery query(m_db);
    if (!query.exec("DELETE FROM history")) {
        qWarning() << "Failed to clear history:" << query.lastError().text();
    }

    QDir imgDir(m_dataPath + "/history_images");
    imgDir.setFilter(QDir::Files);
    foreach(QFileInfo fileInfo, imgDir.entryInfoList()) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}

#endif // HISTORYMANAGER_H
