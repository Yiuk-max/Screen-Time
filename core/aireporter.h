#ifndef AIREPORTER_H
#define AIREPORTER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include "database.h"

class AIReporter : public QObject
{
    Q_OBJECT

public:
    explicit AIReporter(QObject *parent = nullptr);

    void setApiKey(const QString &apiKey);
    void setApiEndpoint(const QString &endpoint);
    void setModel(const QString &model);
    void setPromptTemplate(const QString &prompt);

    void generateWeeklyReport(const QList<UsageRecord> &thisWeekRecords,
                              const QList<UsageRecord> &previousWeekRecords = {});

signals:
    void reportGenerated(const QString &report);
    void reportFailed(const QString &error);
    void requestProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onReplyFinished();

private:
    QString buildPrompt(const QList<UsageRecord> &thisWeekRecords,
                        const QList<UsageRecord> &previousWeekRecords) const;
    QString formatRecordsAsText(const QList<UsageRecord> &records, const QString &label) const;
    QString formatWeekComparison(int thisWeekSeconds, int previousWeekSeconds) const;
    int totalDurationSeconds(const QList<UsageRecord> &records) const;

    QNetworkAccessManager m_networkManager;
    QString m_apiKey;
    QString m_apiEndpoint;
    QString m_model;
    QString m_promptTemplate;
    QNetworkReply *m_currentReply = nullptr;
};

#endif // AIREPORTER_H
