#include "aireporter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

AIReporter::AIReporter(QObject *parent)
    : QObject(parent)
{
    m_apiEndpoint = QStringLiteral("https://api.deepseek.com/v1/chat/completions");
    m_model = QStringLiteral("deepseek-chat");
    m_promptTemplate = QStringLiteral(
        "你是一位专业的时间管理分析师。请基于用户提供的应用使用数据，生成一份简洁的中文周报。\n\n"
        "要求：\n"
        "1. 必须使用下方「程序已计算的环比」原文，不要自行编造增减百分比。\n"
        "2. 列出本周使用最多的 3 个应用。\n"
        "3. 简要点评使用习惯，2-4 句话即可。\n\n"
        "程序已计算的环比：\n%1\n\n"
        "数据如下：\n\n%2"
    );
}

void AIReporter::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void AIReporter::setApiEndpoint(const QString &endpoint)
{
    Q_UNUSED(endpoint);
}

void AIReporter::setModel(const QString &model)
{
    Q_UNUSED(model);
}

void AIReporter::setPromptTemplate(const QString &prompt)
{
    m_promptTemplate = prompt;
}

void AIReporter::generateWeeklyReport(const QList<UsageRecord> &thisWeekRecords,
                                      const QList<UsageRecord> &previousWeekRecords)
{
    if (m_apiKey.isEmpty()) {
        emit reportFailed(QStringLiteral("API Key 未设置"));
        return;
    }

    if (thisWeekRecords.isEmpty()) {
        emit reportFailed(QStringLiteral("没有可用的使用记录数据"));
        return;
    }

    const QString prompt = buildPrompt(thisWeekRecords, previousWeekRecords);

    QJsonObject message;
    message[QStringLiteral("role")] = QStringLiteral("user");
    message[QStringLiteral("content")] = prompt;

    QJsonArray messages;
    messages.append(message);

    QJsonObject requestBody;
    requestBody[QStringLiteral("model")] = m_model;
    requestBody[QStringLiteral("messages")] = messages;
    requestBody[QStringLiteral("temperature")] = 0.7;
    requestBody[QStringLiteral("max_tokens")] = 1000;

    QNetworkRequest request{QUrl(m_apiEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());

    const QByteArray jsonData = QJsonDocument(requestBody).toJson();

    m_currentReply = m_networkManager.post(request, jsonData);
    connect(m_currentReply, &QNetworkReply::finished, this, &AIReporter::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &AIReporter::requestProgress);
}

void AIReporter::onReplyFinished()
{
    if (!m_currentReply) {
        return;
    }

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit reportFailed(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    const QByteArray data = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        emit reportFailed(QStringLiteral("解析响应失败: %1").arg(error.errorString()));
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.contains(QStringLiteral("error"))) {
        const QJsonObject errObj = obj.value(QStringLiteral("error")).toObject();
        const QString message = errObj.value(QStringLiteral("message")).toString();
        emit reportFailed(message.isEmpty() ? QStringLiteral("API 请求失败") : message);
        return;
    }

    const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();

    if (choices.isEmpty()) {
        emit reportFailed(QStringLiteral("AI 返回结果为空"));
        return;
    }

    const QJsonObject choice = choices.first().toObject();
    const QJsonObject message = choice.value(QStringLiteral("message")).toObject();
    const QString content = message.value(QStringLiteral("content")).toString();

    if (content.isEmpty()) {
        emit reportFailed(QStringLiteral("AI 返回内容为空"));
        return;
    }

    emit reportGenerated(content);
}

QString AIReporter::buildPrompt(const QList<UsageRecord> &thisWeekRecords,
                                const QList<UsageRecord> &previousWeekRecords) const
{
    const int thisWeekSeconds = totalDurationSeconds(thisWeekRecords);
    const int previousWeekSeconds = totalDurationSeconds(previousWeekRecords);
    const QString comparison = formatWeekComparison(thisWeekSeconds, previousWeekSeconds);

    QStringList dataSections;
    dataSections << formatRecordsAsText(thisWeekRecords, QStringLiteral("本周"));
    if (!previousWeekRecords.isEmpty()) {
        dataSections << formatRecordsAsText(previousWeekRecords, QStringLiteral("上周"));
    } else {
        dataSections << QStringLiteral("上周：无记录");
    }

    return m_promptTemplate.arg(comparison, dataSections.join(QStringLiteral("\n\n")));
}

int AIReporter::totalDurationSeconds(const QList<UsageRecord> &records) const
{
    int total = 0;
    for (const UsageRecord &record : records) {
        total += record.durationSeconds;
    }
    return total;
}

QString AIReporter::formatWeekComparison(int thisWeekSeconds, int previousWeekSeconds) const
{
    const auto formatDuration = [](int seconds) {
        const int hours = seconds / 3600;
        const int minutes = (seconds % 3600) / 60;
        return QStringLiteral("%1小时%2分钟").arg(hours).arg(minutes);
    };

    const QString thisWeekText = formatDuration(thisWeekSeconds);
    if (previousWeekSeconds <= 0) {
        return QStringLiteral("本周总时长 %1；上周无使用记录，无法计算环比。").arg(thisWeekText);
    }

    const qint64 diff = static_cast<qint64>(thisWeekSeconds) - previousWeekSeconds;
    const int pct = static_cast<int>((diff * 100) / previousWeekSeconds);
    const QString previousWeekText = formatDuration(previousWeekSeconds);

    if (pct > 0) {
        return QStringLiteral("本周总时长 %1，上周总时长 %2；相比上周多了 %3%。")
            .arg(thisWeekText, previousWeekText)
            .arg(pct);
    }
    if (pct < 0) {
        return QStringLiteral("本周总时长 %1，上周总时长 %2；相比上周少了 %3%。")
            .arg(thisWeekText, previousWeekText)
            .arg(-pct);
    }
    return QStringLiteral("本周总时长 %1，上周总时长 %2；与上周持平。").arg(thisWeekText, previousWeekText);
}

QString AIReporter::formatRecordsAsText(const QList<UsageRecord> &records, const QString &label) const
{
    QStringList lines;
    QHash<QString, int> appTotals;

    for (const UsageRecord &record : records) {
        appTotals[record.appName] += record.durationSeconds;
    }

    QList<QPair<QString, int>> sortedApps;
    for (auto it = appTotals.begin(); it != appTotals.end(); ++it) {
        sortedApps.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sortedApps.begin(), sortedApps.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    lines.append(QStringLiteral("%1应用使用总览（按时长排序，前10个）：").arg(label));
    if (sortedApps.isEmpty()) {
        lines.append(QStringLiteral("- 无记录"));
    } else {
        for (int i = 0; i < qMin(10, sortedApps.size()); ++i) {
            const int hours = sortedApps[i].second / 3600;
            const int minutes = (sortedApps[i].second % 3600) / 60;
            lines.append(QStringLiteral("- %1: %2小时%3分钟").arg(sortedApps[i].first).arg(hours).arg(minutes));
        }
    }

    return lines.join(QStringLiteral("\n"));
}
