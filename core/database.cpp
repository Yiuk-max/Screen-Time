#include "database.h"

#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <algorithm>

namespace {

struct DbCandidate {
    QString path;
    qint64 totalSeconds = 0;
    int recordCount = 0;
};

QString canonicalDataDirectory()
{
    const QString roaming = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return QDir(roaming).absoluteFilePath(QStringLiteral("ScreenTime"));
}

QString canonicalDatabasePath()
{
    return QDir(canonicalDataDirectory()).absoluteFilePath(QStringLiteral("screen_time.db"));
}

bool isDatabaseFileName(const QString &fileName)
{
    return fileName.startsWith(QStringLiteral("screen_time"))
        && fileName.contains(QStringLiteral(".db"));
}

QStringList discoverDatabasePaths()
{
    QStringList paths;
    const QString root = canonicalDataDirectory();

    QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo info(it.next());
        if (isDatabaseFileName(info.fileName())) {
            paths << info.absoluteFilePath();
        }
    }

    paths.removeDuplicates();
    return paths;
}

QString toSqliteAttachPath(const QString &path)
{
    return QDir::fromNativeSeparators(path).replace(QStringLiteral("'"), QStringLiteral("''"));
}

bool ensureUsageRecordsTable(QSqlDatabase &db)
{
    if (!db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
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

    query.exec("ALTER TABLE usage_records ADD COLUMN app_path TEXT");
    query.exec("ALTER TABLE usage_records ADD COLUMN tracked_at TEXT");
    return true;
}

bool readDatabaseStats(const QString &path, DbCandidate &out)
{
    out = DbCandidate{};
    out.path = path;

    if (!QFile::exists(path)) {
        return false;
    }

    const QString connectionName = QStringLiteral("stats_%1").arg(qHash(path));
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(path);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(connectionName);
            return false;
        }

        if (!ensureUsageRecordsTable(db)) {
            db.close();
            QSqlDatabase::removeDatabase(connectionName);
            return false;
        }

        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("SELECT COUNT(*), COALESCE(SUM(duration_seconds), 0) FROM usage_records"))) {
            db.close();
            QSqlDatabase::removeDatabase(connectionName);
            return false;
        }

        if (query.next()) {
            out.recordCount = query.value(0).toInt();
            out.totalSeconds = query.value(1).toLongLong();
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return true;
}

bool isRicherThan(const DbCandidate &left, const DbCandidate &right)
{
    if (left.totalSeconds != right.totalSeconds) {
        return left.totalSeconds > right.totalSeconds;
    }
    return left.recordCount > right.recordCount;
}

bool mergeDatabaseInto(const QString &sourcePath, const QString &targetPath)
{
    if (!QFile::exists(sourcePath) || sourcePath == targetPath) {
        return true;
    }

    const QString connectionName = QStringLiteral("merge_main");
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(targetPath);
    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    if (!ensureUsageRecordsTable(db)) {
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    QSqlQuery query(db);
    const QString attachSql = QStringLiteral("ATTACH DATABASE '%1' AS src").arg(toSqliteAttachPath(sourcePath));
    if (!query.exec(attachSql)) {
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    const bool merged = query.exec(
        "INSERT INTO usage_records (date, app_name, window_title, app_path, duration_seconds, tracked_at) "
        "SELECT s.date, s.app_name, s.window_title, s.app_path, s.duration_seconds, s.tracked_at "
        "FROM src.usage_records s "
        "WHERE NOT EXISTS ("
        "  SELECT 1 FROM usage_records u "
        "  WHERE u.date = s.date "
        "    AND u.app_name = s.app_name "
        "    AND COALESCE(u.window_title, '') = COALESCE(s.window_title, '') "
        "    AND COALESCE(u.app_path, '') = COALESCE(s.app_path, '') "
        "    AND u.duration_seconds = s.duration_seconds "
        "    AND COALESCE(u.tracked_at, '') = COALESCE(s.tracked_at, '')"
        ")");

    query.exec(QStringLiteral("DETACH DATABASE src"));
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return merged;
}

bool copyDatabaseFileIfRicher(const QString &sourcePath, const QString &targetPath)
{
    DbCandidate sourceStats;
    if (!readDatabaseStats(sourcePath, sourceStats)) {
        return false;
    }

    if (QFile::exists(targetPath)) {
        DbCandidate targetStats;
        if (!readDatabaseStats(targetPath, targetStats)) {
            qWarning() << "Skip database copy because target stats are unreadable:" << targetPath;
            return false;
        }
        if (!isRicherThan(sourceStats, targetStats)) {
            return false;
        }
    }

    QDir().mkpath(QFileInfo(targetPath).absolutePath());
    if (QFile::exists(targetPath)) {
        const QString backupPath = targetPath + QStringLiteral(".bak");
        QFile::remove(backupPath);
        QFile::rename(targetPath, backupPath);
    }

    if (!QFile::copy(sourcePath, targetPath)) {
        qWarning() << "Failed to copy database from" << sourcePath << "to" << targetPath;
        return false;
    }

    return true;
}

void archiveDatabaseFile(const QString &path)
{
    if (!QFile::exists(path)) {
        return;
    }

    const QString archivedPath = path + QStringLiteral(".merged");
    QFile::remove(archivedPath);
    if (!QFile::rename(path, archivedPath)) {
        QFile::remove(path);
    }
}

void syncLegacyDatabaseMirrors(const QString &canonicalPath)
{
    const QString nestedDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString nestedPath = QDir(nestedDir).absoluteFilePath(QStringLiteral("screen_time.db"));

    if (QFileInfo(nestedPath).canonicalFilePath() == QFileInfo(canonicalPath).canonicalFilePath()) {
        return;
    }

    copyDatabaseFileIfRicher(canonicalPath, nestedPath);
}

QString consolidateDatabaseFiles()
{
    const QString targetPath = canonicalDatabasePath();
    QDir().mkpath(canonicalDataDirectory());

    QList<DbCandidate> candidates;
    for (const QString &path : discoverDatabasePaths()) {
        if (!QFile::exists(path)) {
            continue;
        }

        DbCandidate candidate;
        if (readDatabaseStats(path, candidate)) {
            candidates.append(candidate);
        } else {
            qWarning() << "Skip unreadable database candidate:" << path;
        }
    }

    if (candidates.isEmpty()) {
        return targetPath;
    }

    std::sort(candidates.begin(), candidates.end(), [](const DbCandidate &a, const DbCandidate &b) {
        if (a.totalSeconds != b.totalSeconds) {
            return a.totalSeconds > b.totalSeconds;
        }
        if (a.recordCount != b.recordCount) {
            return a.recordCount > b.recordCount;
        }
        return a.path.length() < b.path.length();
    });

    const DbCandidate primary = candidates.first();
    qInfo() << "Primary database candidate:" << primary.path
            << "records=" << primary.recordCount
            << "totalSeconds=" << primary.totalSeconds;

    if (primary.path != targetPath) {
        copyDatabaseFileIfRicher(primary.path, targetPath);
    }

    DbCandidate targetStats;
    if (!readDatabaseStats(targetPath, targetStats)) {
        qWarning() << "Canonical database is unreadable:" << targetPath;
        return primary.path;
    }

    for (const DbCandidate &candidate : candidates) {
        if (QFileInfo(candidate.path).canonicalFilePath() == QFileInfo(targetPath).canonicalFilePath()) {
            continue;
        }

        mergeDatabaseInto(candidate.path, targetPath);
        if (!candidate.path.endsWith(QStringLiteral(".merged"), Qt::CaseInsensitive)
            && !candidate.path.endsWith(QStringLiteral(".bak"), Qt::CaseInsensitive)) {
            archiveDatabaseFile(candidate.path);
        }
    }

    syncLegacyDatabaseMirrors(targetPath);
    return targetPath;
}

} // namespace

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

    const QString dbPath = consolidateDatabaseFiles();

    if (QSqlDatabase::contains(QStringLiteral("screentime_main"))) {
        QSqlDatabase::removeDatabase(QStringLiteral("screentime_main"));
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("screentime_main"));
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open database:" << dbPath << m_db.lastError().text();
        return false;
    }

    DbCandidate stats;
    if (readDatabaseStats(dbPath, stats)) {
        qInfo() << "Using database:" << dbPath
                << "records=" << stats.recordCount
                << "totalSeconds=" << stats.totalSeconds;
    } else {
        qInfo() << "Using database:" << dbPath;
    }

    return ensureTable();
}

bool Database::ensureTable()
{
    return ensureUsageRecordsTable(m_db);
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
    const QString startDate = QDate::currentDate().addDays(-6).toString(QStringLiteral("yyyy-MM-dd"));
    return querySinceDate(startDate);
}

QList<UsageRecord> Database::queryPreviousWeek() const
{
    const QDate today = QDate::currentDate();
    const QString startDate = today.addDays(-13).toString(QStringLiteral("yyyy-MM-dd"));
    const QString endDate = today.addDays(-7).toString(QStringLiteral("yyyy-MM-dd"));

    QList<UsageRecord> records;
    if (!m_db.isOpen()) {
        return records;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, date, app_name, window_title, COALESCE(app_path, ''), duration_seconds, "
                  "COALESCE(tracked_at, date || ' 00:00:00') "
                  "FROM usage_records WHERE date >= :start_date AND date <= :end_date");
    query.bindValue(":start_date", startDate);
    query.bindValue(":end_date", endDate);
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

QList<UsageRecord> Database::querySinceDate(const QString &startDate) const
{
    QList<UsageRecord> records;
    if (!m_db.isOpen()) {
        return records;
    }

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
