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
    // 设置 AI 报告的输出语言（英文语言名，如 "German"）。空或中文时输出中文。
    void setOutputLanguage(const QString &languageName);

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
    QString systemPromptForKind(AIReportKind kind) const;
    QString outputLanguageDirective() const;
    bool isChineseOutput() const;

    QNetworkAccessManager m_networkManager;
    QString m_apiKey;
    QString m_apiEndpoint;
    QString m_model;
    QString m_promptTemplate;
    QString m_outputLanguage = QStringLiteral("简体中文");
    QNetworkReply *m_currentReply = nullptr;
};

#endif // AIREPORTER_H
