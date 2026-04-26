#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QImage>
#include <QSqlDatabase>
#include <QDateTime>
#include "copyprocessor.h"

struct HistoryRecord {
    int id = -1;                      // 数据库自增 ID
    QString imageHash;                // 图片 MD5 哈希
    QString cachedImagePath;          // 缓存的图片副本路径
    QString resultText;               // 识别结果
    ContentType recognitionType;      // 识别类型
    QDateTime timestamp;              // 时间戳
};

class HistoryManager : public QObject
{
    Q_OBJECT
public:
    static HistoryManager* instance();

    void init(); // 初始化数据库连接和表结构

    // 保存或更新记录（根据图片内容哈希判断）
    void saveRecord(const QImage& image, const QString& result, ContentType type);

    // 获取最近的记录列表（分页）
    // limit: 每页条数，-1 表示使用配置值；配置值为 0 表示无限制
    // offset: 跳过的记录数（用于分页偏移）
    QList<HistoryRecord> getRecentRecords(int limit = -1, int offset = 0);

    // 【新增】获取历史记录总条数
    int getTotalCount();

    // 根据 ID 删除记录
    void deleteRecord(int id);

    // 清空所有历史
    void clearAll();

    // 根据 ID 获取单条记录详情
    HistoryRecord getRecordById(int id);

private:
    explicit HistoryManager(QObject *parent = nullptr);
    bool ensureTableExists();
    QString hashImage(const QImage& image) const;
    QString saveImageCache(const QImage& image, const QString& hash);

    QSqlDatabase m_db;
    QString m_dataPath;
};

#endif // HISTORYMANAGER_H
