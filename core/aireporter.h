#ifndef AIREPORTER_H
#define AIREPORTER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include "database.h"

enum class AIReportKind {
    Daily,
    Weekly
};

class AIReporter : public QObject
{
    Q_OBJECT

public:
    explicit AIReporter(QObject *parent = nullptr);

    void setApiKey(const QString &apiKey);
    void setApiEndpoint(const QString &endpoint);
    void setModel(const QString &model);
    void setPromptTemplate(const QString &prompt);

    void generateReport(AIReportKind kind,
                        const QList<UsageRecord> &records,
                        const QList<UsageRecord> &compareRecords = {});

    void generateWeeklyReport(const QList<UsageRecord> &thisWeekRecords,
                              const QList<UsageRecord> &previousWeekRecords = {});

signals:
    void reportGenerated(const QString &report);
    void reportFailed(const QString &error);
    void requestProgress(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onReplyFinished();

private:
    QString buildPrompt(AIReportKind kind,
                        const QList<UsageRecord> &records,
                        const QList<UsageRecord> &compareRecords) const;
    QString buildStatisticsBlock(AIReportKind kind,
                                 const QList<UsageRecord> &records,
                                 const QList<UsageRecord> &compareRecords) const;
    QString formatWeekComparison(int thisSeconds, int previousSeconds) const;
    int totalDurationSeconds(const QList<UsageRecord> &records) const;
    static QString systemPromptForKind(AIReportKind kind);

    QNetworkAccessManager m_networkManager;
    QString m_apiKey;
    QString m_apiEndpoint;
    QString m_model;
    QString m_promptTemplate;
    QNetworkReply *m_currentReply = nullptr;
};

#endif // AIREPORTER_H
