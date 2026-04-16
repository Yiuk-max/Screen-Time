#include "database.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

Database::Database()
{
}

Database::~Database()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::init()
{
    if (m_db.isOpen()) {
        return true;
    }

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString dbPath = dataDir + "/screen_time.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        return false;
    }

    return ensureTable();
}

bool Database::ensureTable()
{
    QSqlQuery query(m_db);
    const bool created = query.exec(
        "CREATE TABLE IF NOT EXISTS usage_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "date TEXT NOT NULL,"
        "app_name TEXT NOT NULL,"
        "window_title TEXT,"
        "app_path TEXT,"
        "duration_seconds INTEGER NOT NULL,"
        "tracked_at TEXT"
        ")");
    if (!created) {
        return false;
    }

    // Compatible migration for old table without tracked_at.
    query.exec("ALTER TABLE usage_records ADD COLUMN app_path TEXT");
    query.exec("ALTER TABLE usage_records ADD COLUMN tracked_at TEXT");
    return true;
}

bool Database::addRecord(const QString &date,
                         const QString &appName,
                         const QString &windowTitle,
                         const QString &appPath,
                         int durationSeconds,
                         const QString &trackedAt)
{
    if (!m_db.isOpen() && !const_cast<Database *>(this)->init()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO usage_records (date, app_name, window_title, app_path, duration_seconds, tracked_at) "
                  "VALUES (:date, :app_name, :window_title, :app_path, :duration_seconds, :tracked_at)");
    query.bindValue(":date", date);
    query.bindValue(":app_name", appName);
    query.bindValue(":window_title", windowTitle);
    query.bindValue(":app_path", appPath);
    query.bindValue(":duration_seconds", durationSeconds);
    query.bindValue(":tracked_at", trackedAt);
    return query.exec();
}

QList<UsageRecord> Database::queryToday() const
{
    QList<UsageRecord> records;
    if (!m_db.isOpen()) {
        return records;
    }

    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query(m_db);
    query.prepare("SELECT id, date, app_name, window_title, COALESCE(app_path, ''), duration_seconds, "
                  "COALESCE(tracked_at, date || ' 00:00:00') "
                  "FROM usage_records WHERE date = :date");
    query.bindValue(":date", today);
    if (!query.exec()) {
        return records;
    }

    while (query.next()) {
        UsageRecord record;
        record.id = query.value(0).toInt();
        record.date = query.value(1).toString();
        record.appName = query.value(2).toString();
        record.windowTitle = query.value(3).toString();
        record.appPath = query.value(4).toString();
        record.durationSeconds = query.value(5).toInt();
        record.trackedAt = query.value(6).toString();
        records.append(record);
    }
    return records;
}

QList<UsageRecord> Database::queryWeekly() const
{
    QList<UsageRecord> records;
    if (!m_db.isOpen()) {
        return records;
    }

    const QString startDate = QDate::currentDate().addDays(-6).toString("yyyy-MM-dd");
    QSqlQuery query(m_db);
    query.prepare("SELECT id, date, app_name, window_title, COALESCE(app_path, ''), duration_seconds, "
                  "COALESCE(tracked_at, date || ' 00:00:00') "
                  "FROM usage_records WHERE date >= :start_date");
    query.bindValue(":start_date", startDate);
    if (!query.exec()) {
        return records;
    }

    while (query.next()) {
        UsageRecord record;
        record.id = query.value(0).toInt();
        record.date = query.value(1).toString();
        record.appName = query.value(2).toString();
        record.windowTitle = query.value(3).toString();
        record.appPath = query.value(4).toString();
        record.durationSeconds = query.value(5).toInt();
        record.trackedAt = query.value(6).toString();
        records.append(record);
    }
    return records;
}
