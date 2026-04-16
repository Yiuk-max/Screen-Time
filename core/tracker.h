#ifndef TRACKER_H
#define TRACKER_H

#include <QObject>
#include <QTimer>

#include "database.h"

class Tracker : public QObject
{
    Q_OBJECT

public:
    explicit Tracker(Database *database, QObject *parent = nullptr);

    void start();
    void stop();

private slots:
    void onTick();

private:
    void flushToDatabase();
    QString currentAppPath() const;
    QString currentAppName() const;
    QString currentWindowTitle() const;

    Database *m_database = nullptr;
    QTimer m_tickTimer;

    QString m_lastAppName;
    QString m_lastAppPath;
    QString m_lastWindowTitle;
    int m_accumulatedSeconds = 0;
    int m_secondsSinceLastFlush = 0;
};

#endif // TRACKER_H
