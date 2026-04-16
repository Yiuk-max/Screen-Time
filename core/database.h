#ifndef DATABASE_H
#define DATABASE_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

struct UsageRecord {
    int id = -1;
    QString date;
    QString appName;
    QString windowTitle;
    QString appPath;
    QString trackedAt;
    int durationSeconds = 0;
};

class Database
{
public:
    Database();
    ~Database();

    bool init();

    bool addRecord(const QString &date,
                   const QString &appName,
                   const QString &windowTitle,
                   const QString &appPath,
                   int durationSeconds,
                   const QString &trackedAt);

    QList<UsageRecord> queryToday() const;
    QList<UsageRecord> queryWeekly() const;

private:
    bool ensureTable();
    QSqlDatabase m_db;
};

#endif // DATABASE_H
