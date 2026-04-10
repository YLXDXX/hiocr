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
    // image_hash 作为唯一键，确保同一图片内容只有一条记录
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "image_hash TEXT UNIQUE, "
        "cached_image_path TEXT, "
        "result_text TEXT, "
        "recognition_type INTEGER, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
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

    QString hash = hashImage(image);
    QString cachedPath = saveImageCache(image, hash);

    QSqlQuery query(m_db);

    // 使用 INSERT OR REPLACE 语法 (UPSERT)
    // 如果 image_hash 已存在，则替换整行数据（包括时间戳，达到更新效果）
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

    // 【新增】自动清理逻辑
    int limit = SettingsManager::instance()->historyLimit();
    if (limit > 0) {
        // 1. 获取当前总数
        QSqlQuery countQuery(m_db);
        countQuery.exec("SELECT COUNT(*) FROM history");
        if (countQuery.next() && countQuery.value(0).toInt() > limit) {
            // 2. 计算需要删除的数量
            int deleteCount = countQuery.value(0).toInt() - limit;

            // 3. 查找最旧的记录 ID 和图片路径
            QSqlQuery findOldQuery(m_db);
            findOldQuery.prepare("SELECT id, cached_image_path FROM history ORDER BY timestamp ASC LIMIT ?");
            findOldQuery.addBindValue(deleteCount);

            if (findOldQuery.exec()) {
                QStringList ids;
                QStringList paths;
                while (findOldQuery.next()) {
                    ids << QString::number(findOldQuery.value(0).toInt());
                    paths << findOldQuery.value(1).toString();
                }

                // 4. 删除数据库记录
                if (!ids.isEmpty()) {
                    QSqlQuery deleteQuery(m_db);
                    deleteQuery.exec(QString("DELETE FROM history WHERE id IN (%1)").arg(ids.join(",")));

                    // 5. 删除图片文件
                    for (const QString& path : paths) {
                        QFile::remove(path);
                    }
                    qDebug() << "History cleanup: removed" << deleteCount << "old records.";
                }
            }
        }
    }
}

QList<HistoryRecord> HistoryManager::getRecentRecords(int limit)
{
    QList<HistoryRecord> list;
    QSqlQuery query(m_db);

    // 【修改】SQL 查询中增加 result_text
    query.prepare("SELECT id, cached_image_path, result_text, recognition_type, timestamp FROM history ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            HistoryRecord record;
            record.id = query.value(0).toInt();
            record.cachedImagePath = query.value(1).toString();
            record.resultText = query.value(2).toString();         // 【新增】读取结果文本
            record.recognitionType = static_cast<ContentType>(query.value(3).toInt());
            record.timestamp = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
            list.append(record);
        }
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
        record.timestamp = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
    }
    return record;
}

void HistoryManager::deleteRecord(int id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM history WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        // 可选：此处可以顺便清理孤儿图片文件，但为了性能可以暂不清理，留待清理机制
    }
}

void HistoryManager::clearAll()
{
    QSqlQuery query(m_db);
    query.exec("DELETE FROM history");

    // 清空图片缓存目录
    QDir imgDir(m_dataPath + "/history_images");
    imgDir.setFilter(QDir::Files);
    foreach (QFileInfo fileInfo, imgDir.entryInfoList()) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}
