#include "historymanager.h"
#include "settingsmanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QBuffer>
#include <QDebug>
#include <QDateTime>

HistoryManager* HistoryManager::instance()
{
    static HistoryManager* s_instance = nullptr;
    if (!s_instance) s_instance = new HistoryManager();
    return s_instance;
}

HistoryManager::HistoryManager(QObject *parent) : QObject(parent)
{
    // 设置数据存储路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) dir.mkpath(".");

    m_dataPath = configPath;
}

void HistoryManager::init()
{
    // 数据库文件路径
    QString dbPath = m_dataPath + "/history.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE", "history_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open history database:" << m_db.lastError().text();
        return;
    }

    ensureTableExists();

    // 创建图片缓存目录
    QDir imgDir(m_dataPath + "/history_images");
    if (!imgDir.exists()) imgDir.mkpath(".");
}

bool HistoryManager::ensureTableExists()
{
    QSqlQuery query(m_db);
    // 【修复】DEFAULT 时间戳使用 localtime，与 INSERT 语句保持一致
    // 避免 UTC 与本地时间混用导致排序错误、清理时误删记录
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

QString HistoryManager::hashImage(const QImage& image) const
{
    if (image.isNull()) return QString();

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG"); // 保存为 PNG 保证一致性
    buffer.close();

    return QCryptographicHash::hash(byteArray, QCryptographicHash::Md5).toHex();
}

QString HistoryManager::saveImageCache(const QImage& image, const QString& hash)
{
    QString fileName = m_dataPath + "/history_images/" + hash + ".png";

    // 如果文件已存在，直接返回路径，避免重复写入
    if (QFile::exists(fileName)) return fileName;

    if (image.save(fileName, "PNG")) {
        return fileName;
    }
    return QString();
}

void HistoryManager::saveRecord(const QImage& image, const QString& result, ContentType type)
{
    if (image.isNull() || result.isEmpty()) return;

    // 【修复】增加 saveHistoryEnabled 防御性检查
    // 即使调用方已检查，HistoryManager 自身也应保证一致性
    if (!SettingsManager::instance()->saveHistoryEnabled()) return;

    QString hash = hashImage(image);
    QString cachedPath = saveImageCache(image, hash);

    QSqlQuery query(m_db);

    // 时间戳统一使用 localtime，与建表 DEFAULT 一致
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

    // 自动清理逻辑
    int limit = SettingsManager::instance()->historyLimit();
    // limit == 0 表示无限制，跳过清理
    if (limit > 0) {
        // 1. 获取当前总数
        QSqlQuery countQuery(m_db);
        if (!countQuery.exec("SELECT COUNT(*) FROM history")) {
            qWarning() << "Failed to count history:" << countQuery.lastError().text();
            return;
        }
        if (countQuery.next() && countQuery.value(0).toInt() > limit) {
            // 2. 计算需要删除的数量
            int deleteCount = countQuery.value(0).toInt() - limit;

            // 3. 查找最旧的记录 ID 和图片路径
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

                // 4. 删除数据库记录
                if (!ids.isEmpty()) {
                    // 【修复】使用参数化查询，防止 SQL 注入风险
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

                    // 5. 删除关联的图片文件
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

QList<HistoryRecord> HistoryManager::getRecentRecords(int limit)
{
    // 【修复】limit == -1 时使用配置值；配置值为 0 表示无限制
    if (limit < 0) {
        int configuredLimit = SettingsManager::instance()->historyLimit();
        limit = (configuredLimit == 0) ? 999999 : configuredLimit;
    }

    QList<HistoryRecord> list;
    QSqlQuery query(m_db);

    query.prepare("SELECT id, cached_image_path, result_text, recognition_type, timestamp FROM history ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            HistoryRecord record;
            record.id = query.value(0).toInt();
            record.cachedImagePath = query.value(1).toString();
            record.resultText = query.value(2).toString();
            record.recognitionType = static_cast<ContentType>(query.value(3).toInt());
            // 【修复】使用与 SQLite datetime('now','localtime') 输出匹配的格式
            // SQLite 输出格式为 "2024-01-15 14:30:00"（空格分隔），
            // Qt::ISODate 期望 "2024-01-15T14:30:00"（T 分隔），会导致解析失败
            record.timestamp = QDateTime::fromString(query.value(4).toString(), "yyyy-MM-dd hh:mm:ss");
            list.append(record);
        }
    } else {
        qWarning() << "Failed to query recent records:" << query.lastError().text();
    }
    return list;
}

HistoryRecord HistoryManager::getRecordById(int id)
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
        // 【修复】同 getRecentRecords，使用匹配的格式字符串
        record.timestamp = QDateTime::fromString(query.value(4).toString(), "yyyy-MM-dd hh:mm:ss");
    } else {
        if (!query.exec()) {
            qWarning() << "Failed to get record by id:" << query.lastError().text();
        }
    }
    return record;
}

void HistoryManager::deleteRecord(int id)
{
    // 【修复】删除记录前先获取关联的图片路径，以便清理图片文件
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
        // 【修复】删除关联的图片缓存文件，避免孤儿文件占用磁盘
        if (!imagePath.isEmpty()) {
            QFile::remove(imagePath);
        }
    } else {
        qWarning() << "Failed to delete record:" << query.lastError().text();
    }
}

void HistoryManager::clearAll()
{
    QSqlQuery query(m_db);
    if (!query.exec("DELETE FROM history")) {
        qWarning() << "Failed to clear history:" << query.lastError().text();
    }

    // 清空图片缓存目录
    QDir imgDir(m_dataPath + "/history_images");
    imgDir.setFilter(QDir::Files);
    foreach(QFileInfo fileInfo, imgDir.entryInfoList()) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}
