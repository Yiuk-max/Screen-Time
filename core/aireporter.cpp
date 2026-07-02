#include "aireporter.h"
#include "appnameresolver.h"

#include <QDate>
#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>

namespace {

struct AppAggregate {
    QString processName;
    QString samplePath;
    QString sampleTitle;
    qint64 seconds = 0;
};

QString formatDuration(int seconds)
{
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;
    if (hours > 0) {
        return QStringLiteral("%1小时%2分钟").arg(hours).arg(minutes);
    }
    if (minutes > 0) {
        return QStringLiteral("%1分钟").arg(minutes);
    }
    return QStringLiteral("不足1分钟");
}

QString normalizeProcessKey(const QString &appName, const QString &appPath)
{
    if (!appPath.isEmpty()) {
        const QFileInfo info(appPath);
        if (!info.fileName().isEmpty()) {
            return info.fileName().toLower();
        }
    }
    return appName.toLower();
}

QString processDisplayBase(const QString &appName, const QString &appPath)
{
    if (!appPath.isEmpty()) {
        const QFileInfo info(appPath);
        if (!info.fileName().isEmpty()) {
            return info.fileName();
        }
    }
    return appName;
}

int hourFromTrackedAt(const QString &trackedAt)
{
    QDateTime dt = QDateTime::fromString(trackedAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!dt.isValid()) {
        dt = QDateTime::fromString(trackedAt, Qt::ISODate);
    }
    return dt.isValid() ? dt.time().hour() : -1;
}

QString usageCategory(const QString &processKey, const QString &appPath)
{
    const QString blob = (processKey + QLatin1Char(' ') + appPath).toLower();

    if (blob.contains(QStringLiteral("screentime"))) {
        return QStringLiteral("本程序");
    }
    if (blob.contains(QStringLiteral("lockapp"))
        || blob.contains(QStringLiteral("dwm"))
        || blob.contains(QStringLiteral("searchhost"))
        || blob.contains(QStringLiteral("sihost"))
        || blob.contains(QStringLiteral("runtimebroker"))
        || blob.contains(QStringLiteral("applicationframehost"))
        || blob.contains(QStringLiteral("systemsettings"))
        || blob.contains(QStringLiteral("explorer"))) {
        return QStringLiteral("系统");
    }
    if (blob.contains(QStringLiteral("msedge"))
        || blob.contains(QStringLiteral("chrome"))
        || blob.contains(QStringLiteral("firefox"))
        || blob.contains(QStringLiteral("brave"))
        || blob.contains(QStringLiteral("opera"))
        || blob.contains(QStringLiteral("browser"))) {
        return QStringLiteral("浏览器");
    }
    if (blob.contains(QStringLiteral("wechat"))
        || blob.contains(QStringLiteral("weixin"))
        || blob.contains(QStringLiteral("qq"))
        || blob.contains(QStringLiteral("discord"))
        || blob.contains(QStringLiteral("teams"))
        || blob.contains(QStringLiteral("slack"))
        || blob.contains(QStringLiteral("feishu"))
        || blob.contains(QStringLiteral("dingtalk"))
        || blob.contains(QStringLiteral("telegram"))) {
        return QStringLiteral("通讯社交");
    }
    if (blob.contains(QStringLiteral("code"))
        || blob.contains(QStringLiteral("cursor"))
        || blob.contains(QStringLiteral("devenv"))
        || blob.contains(QStringLiteral("idea"))
        || blob.contains(QStringLiteral("pycharm"))
        || blob.contains(QStringLiteral("android studio"))
        || blob.contains(QStringLiteral("git"))) {
        return QStringLiteral("开发工具");
    }
    if (blob.contains(QStringLiteral("winword"))
        || blob.contains(QStringLiteral("excel"))
        || blob.contains(QStringLiteral("powerpnt"))
        || blob.contains(QStringLiteral("wps"))
        || blob.contains(QStringLiteral("notion"))
        || blob.contains(QStringLiteral("obsidian"))) {
        return QStringLiteral("办公文档");
    }
    if (blob.contains(QStringLiteral("steam"))
        || blob.contains(QStringLiteral("game"))
        || blob.contains(QStringLiteral("htgame"))
        || blob.contains(QStringLiteral("genshin"))
        || blob.contains(QStringLiteral("launcher"))) {
        return QStringLiteral("游戏娱乐");
    }
    if (blob.contains(QStringLiteral("bilibili"))
        || blob.contains(QStringLiteral("douyin"))
        || blob.contains(QStringLiteral("spotify"))
        || blob.contains(QStringLiteral("cloudmusic"))
        || blob.contains(QStringLiteral("qqmusic"))
        || blob.contains(QStringLiteral("vlc"))
        || blob.contains(QStringLiteral("obs"))) {
        return QStringLiteral("影音娱乐");
    }
    return QStringLiteral("其他");
}

QHash<QString, AppAggregate> aggregateByProcess(const QList<UsageRecord> &records)
{
    QHash<QString, AppAggregate> map;
    for (const UsageRecord &record : records) {
        const QString key = normalizeProcessKey(record.appName, record.appPath);
        AppAggregate &agg = map[key];
        if (agg.processName.isEmpty()) {
            agg.processName = processDisplayBase(record.appName, record.appPath);
            agg.samplePath = record.appPath;
            agg.sampleTitle = record.windowTitle;
        }
        agg.seconds += record.durationSeconds;
        if (agg.samplePath.isEmpty() && !record.appPath.isEmpty()) {
            agg.samplePath = record.appPath;
        }
        if (agg.sampleTitle.isEmpty() && !record.windowTitle.isEmpty()) {
            agg.sampleTitle = record.windowTitle;
        }
    }
    return map;
}

QList<QPair<QString, qint64>> sortedAppList(const QHash<QString, AppAggregate> &map)
{
    QList<QPair<QString, qint64>> list;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        list.append({it.key(), it.value().seconds});
    }
    std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });
    return list;
}

QString periodLabelForHour(int hour)
{
    if (hour >= 6 && hour < 12) {
        return QStringLiteral("上午(6-12点)");
    }
    if (hour >= 12 && hour < 18) {
        return QStringLiteral("下午(12-18点)");
    }
    if (hour >= 18 && hour < 22) {
        return QStringLiteral("傍晚(18-22点)");
    }
    return QStringLiteral("夜间(22-6点)");
}

QString stripMarkdownForDisplay(const QString &text)
{
    QString out = text;
    out.replace(QRegularExpression(QStringLiteral("^#{1,6}\\s*"), QRegularExpression::MultilineOption), QString());
    out.replace(QStringLiteral("**"), QString());
    out.replace(QStringLiteral("__"), QString());
    out.replace(QStringLiteral("```"), QString());
    out.replace(QRegularExpression(QStringLiteral("^---\\s*$"), QRegularExpression::MultilineOption), QString());
    out.replace(QRegularExpression(QStringLiteral("^\\*\\s+"), QRegularExpression::MultilineOption), QStringLiteral("· "));
    return out.trimmed();
}

} // namespace

AIReporter::AIReporter(QObject *parent)
    : QObject(parent)
{
    m_apiEndpoint = QStringLiteral("https://api.deepseek.com/v1/chat/completions");
    m_model = QStringLiteral("deepseek-chat");
}

void AIReporter::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void AIReporter::setApiEndpoint(const QString &endpoint)
{
    if (!endpoint.isEmpty()) {
        m_apiEndpoint = endpoint;
    }
}

void AIReporter::setModel(const QString &model)
{
    if (!model.isEmpty()) {
        m_model = model;
    }
}

void AIReporter::setPromptTemplate(const QString &prompt)
{
    if (!prompt.isEmpty()) {
        m_promptTemplate = prompt;
    }
}

void AIReporter::generateWeeklyReport(const QList<UsageRecord> &thisWeekRecords,
                                      const QList<UsageRecord> &previousWeekRecords)
{
    generateReport(AIReportKind::Weekly, thisWeekRecords, previousWeekRecords);
}

void AIReporter::generateReport(AIReportKind kind,
                                const QList<UsageRecord> &records,
                                const QList<UsageRecord> &compareRecords)
{
    if (m_apiKey.isEmpty()) {
        emit reportFailed(QStringLiteral("API Key 未设置"));
        return;
    }

    if (records.isEmpty()) {
        emit reportFailed(QStringLiteral("没有可用的使用记录数据"));
        return;
    }

    const QString userPrompt = buildPrompt(kind, records, compareRecords);

    QJsonObject systemMessage;
    systemMessage[QStringLiteral("role")] = QStringLiteral("system");
    systemMessage[QStringLiteral("content")] = systemPromptForKind(kind);

    QJsonObject userMessage;
    userMessage[QStringLiteral("role")] = QStringLiteral("user");
    userMessage[QStringLiteral("content")] = userPrompt;

    QJsonArray messages;
    messages.append(systemMessage);
    messages.append(userMessage);

    QJsonObject requestBody;
    requestBody[QStringLiteral("model")] = m_model;
    requestBody[QStringLiteral("messages")] = messages;
    requestBody[QStringLiteral("temperature")] = 0.4;
    requestBody[QStringLiteral("max_tokens")] = 4096;

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

    emit reportGenerated(stripMarkdownForDisplay(content));
}

QString AIReporter::systemPromptForKind(AIReportKind kind)
{
    const QString reportName = kind == AIReportKind::Daily ? QStringLiteral("日") : QStringLiteral("周");
    return QStringLiteral(
               "你是专业的屏幕使用时间分析师。你必须严格依据用户消息中的「程序预计算数据」撰写中文%1报，"
               "不得编造时长、排名或环比数字。"
               "报告中所有应用名称须写成中文常用名：数据里是 Windows 进程名（如 msedge.exe、LockApp.exe），"
               "请结合路径、窗口标题、本地识别提示与你的知识判断真实软件（例如 msedge=微软 Edge，"
               "LockApp.exe=系统锁屏，HTGame.exe=异环，ScreenTime.exe=本程序）。"
               "对陌生进程请尽量准确推断，勿直接堆砌英文进程名。"
               "输出为纯文本：用「一、二、三」或「1. 2. 3.」分节，禁止 #、**、---、代码块等 Markdown 符号。")
        .arg(reportName);
}

QString AIReporter::buildPrompt(AIReportKind kind,
                                const QList<UsageRecord> &records,
                                const QList<UsageRecord> &compareRecords) const
{
    const int thisSeconds = totalDurationSeconds(records);
    const int compareSeconds = totalDurationSeconds(compareRecords);
    const QString comparison = formatWeekComparison(thisSeconds, compareSeconds);
    const QString stats = buildStatisticsBlock(kind, records, compareRecords);

    if (kind == AIReportKind::Daily) {
        return QStringLiteral(
                   "请根据以下数据撰写「今日屏幕使用分析报告」。\n\n"
                   "【必须包含的章节】\n"
                   "一、总时长概览\n"
                   "二、分类统计（浏览器/开发/办公/游戏/通讯/系统/其他）\n"
                   "三、按小时使用分布与高峰时段\n"
                   "四、Top10 应用列表（必须按下方 Top10 时长排序，写中文软件名）\n"
                   "五、上午(6-12点)主要做什么\n"
                   "六、下午(12-18点)主要做什么\n"
                   "七、使用习惯建议\n\n"
                   "【程序预计算数据】\n%1")
            .arg(stats);
    }

    return QStringLiteral(
               "请根据以下数据撰写「本周屏幕使用分析报告」。\n\n"
               "【必须包含的章节】\n"
               "一、总时长概览\n"
               "二、与上周环比（必须引用下方「程序已计算的环比」原文，并展开说明主要变化）\n"
               "三、分类统计（浏览器/开发/办公/游戏/通讯/系统/其他）\n"
               "四、每日使用趋势（近7天逐日）\n"
               "五、高峰时段（按小时分布）\n"
               "六、Top10 应用列表（必须严格按下方 Top10 排序与时长，写中文软件名）\n"
               "七、上午(6-12点)主要做什么\n"
               "八、下午(12-18点)主要做什么\n"
               "九、是否需要调整的习惯与具体建议\n\n"
               "【程序已计算的环比】\n%1\n\n"
               "【程序预计算数据】\n%2")
        .arg(comparison, stats);
}

QString AIReporter::buildStatisticsBlock(AIReportKind kind,
                                         const QList<UsageRecord> &records,
                                         const QList<UsageRecord> &compareRecords) const
{
    QStringList lines;
    const QHash<QString, AppAggregate> appMap = aggregateByProcess(records);
    const QList<QPair<QString, qint64>> sortedApps = sortedAppList(appMap);

    const int totalSeconds = totalDurationSeconds(records);
    lines << QStringLiteral("总使用时长: %1（共 %2 条原始记录）")
                 .arg(formatDuration(totalSeconds))
                 .arg(records.size());

    if (kind == AIReportKind::Weekly && !compareRecords.isEmpty()) {
        const QHash<QString, AppAggregate> prevMap = aggregateByProcess(compareRecords);
        lines << QStringLiteral("上周总时长: %1").arg(formatDuration(totalDurationSeconds(compareRecords)));
        lines << QStringLiteral("上周记录条数: %1").arg(compareRecords.size());
    }

    // 分类统计
    QHash<QString, qint64> categorySeconds;
    for (auto it = appMap.cbegin(); it != appMap.cend(); ++it) {
        const QString cat = usageCategory(it.key(), it.value().samplePath);
        categorySeconds[cat] += it.value().seconds;
    }
    lines << QString();
    lines << QStringLiteral("【分类统计（程序汇总，请据此撰写）】");
    QList<QPair<QString, qint64>> cats;
    for (auto it = categorySeconds.cbegin(); it != categorySeconds.cend(); ++it) {
        cats.append({it.key(), it.value()});
    }
    std::sort(cats.begin(), cats.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
    for (const auto &c : cats) {
        lines << QStringLiteral("- %1: %2").arg(c.first, formatDuration(static_cast<int>(c.second)));
    }

    // 每日趋势
    if (kind == AIReportKind::Weekly) {
        const QDate endDate = QDate::currentDate();
        const QDate startDate = endDate.addDays(-6);
        QHash<QString, qint64> daySeconds;
        for (const UsageRecord &r : records) {
            daySeconds[r.date] += r.durationSeconds;
        }
        lines << QString();
        lines << QStringLiteral("【每日趋势（近7天）】");
        for (int i = 0; i < 7; ++i) {
            const QDate day = startDate.addDays(i);
            const QString key = day.toString(QStringLiteral("yyyy-MM-dd"));
            const qint64 sec = daySeconds.value(key, 0);
            lines << QStringLiteral("- %1 (%2): %3")
                         .arg(key, day.toString(QStringLiteral("dddd")), formatDuration(static_cast<int>(sec)));
        }
    }

    // 按小时
    QVector<qint64> hourSeconds(24, 0);
    int hourKnownRecords = 0;
    for (const UsageRecord &r : records) {
        const int hour = hourFromTrackedAt(r.trackedAt);
        if (hour >= 0 && hour < 24) {
            hourSeconds[hour] += r.durationSeconds;
            ++hourKnownRecords;
        }
    }
    lines << QString();
    lines << QStringLiteral("【按小时分布（%1 条记录含时间戳）】").arg(hourKnownRecords);
    int peakHour = 0;
    qint64 peakVal = 0;
    for (int h = 0; h < 24; ++h) {
        if (hourSeconds[h] > 0) {
            lines << QStringLiteral("- %1:00-%2:59: %3")
                         .arg(h, 2, 10, QChar('0'))
                         .arg(h, 2, 10, QChar('0'))
                         .arg(formatDuration(static_cast<int>(hourSeconds[h])));
        }
        if (hourSeconds[h] > peakVal) {
            peakVal = hourSeconds[h];
            peakHour = h;
        }
    }
    if (peakVal > 0) {
        lines << QStringLiteral("高峰时段: %1:00-%2:59（%3）")
                     .arg(peakHour, 2, 10, QChar('0'))
                     .arg(peakHour, 2, 10, QChar('0'))
                     .arg(formatDuration(static_cast<int>(peakVal)));
    }

    // 上午 / 下午
    QHash<QString, qint64> morningApps;
    QHash<QString, qint64> afternoonApps;
    for (const UsageRecord &r : records) {
        const int hour = hourFromTrackedAt(r.trackedAt);
        const QString key = normalizeProcessKey(r.appName, r.appPath);
        if (hour >= 6 && hour < 12) {
            morningApps[key] += r.durationSeconds;
        } else if (hour >= 12 && hour < 18) {
            afternoonApps[key] += r.durationSeconds;
        }
    }
    auto formatTopPeriod = [&appMap](const QHash<QString, qint64> &periodMap) {
        QList<QPair<QString, qint64>> items;
        for (auto it = periodMap.cbegin(); it != periodMap.cend(); ++it) {
            items.append({it.key(), it.value()});
        }
        std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
        QStringList out;
        for (int i = 0; i < qMin(5, items.size()); ++i) {
            const AppAggregate agg = appMap.value(items[i].first);
            const QString known = resolveAppDisplayName(agg.processName, agg.samplePath);
            const QString name = known.isEmpty() ? agg.processName : known;
            out << QStringLiteral("%1(%2)").arg(name, formatDuration(static_cast<int>(items[i].second)));
        }
        return out.isEmpty() ? QStringLiteral("无足够分时数据") : out.join(QStringLiteral("、"));
    };
    lines << QString();
    lines << QStringLiteral("【上午 6-12 点 Top 应用（程序汇总）】") << formatTopPeriod(morningApps);
    lines << QStringLiteral("【下午 12-18 点 Top 应用（程序汇总）】") << formatTopPeriod(afternoonApps);

    // Top10 — 权威列表
    lines << QString();
    lines << QStringLiteral("【Top10 应用（按时长降序，报告必须与此一致）】");
    for (int i = 0; i < qMin(10, sortedApps.size()); ++i) {
        const AppAggregate agg = appMap.value(sortedApps[i].first);
        lines << formatProcessHintLine(agg.processName,
                                       agg.samplePath,
                                       static_cast<int>(sortedApps[i].second));
        if (!agg.sampleTitle.isEmpty()) {
            lines << QStringLiteral("  窗口标题示例: %1").arg(agg.sampleTitle);
        }
    }

    // 上周 Top5 对比（周报）
    if (kind == AIReportKind::Weekly && !compareRecords.isEmpty()) {
        const QHash<QString, AppAggregate> prevMap = aggregateByProcess(compareRecords);
        const QList<QPair<QString, qint64>> prevSorted = sortedAppList(prevMap);
        lines << QString();
        lines << QStringLiteral("【上周 Top5 应用（环比参考）】");
        for (int i = 0; i < qMin(5, prevSorted.size()); ++i) {
            const AppAggregate agg = prevMap.value(prevSorted[i].first);
            lines << formatProcessHintLine(agg.processName,
                                           agg.samplePath,
                                           static_cast<int>(prevSorted[i].second));
        }
    }

    // 进程识别提示
    lines << QString();
    lines << QStringLiteral("【进程识别提示（撰写时请用中文名）】");
    for (int i = 0; i < qMin(15, sortedApps.size()); ++i) {
        const AppAggregate agg = appMap.value(sortedApps[i].first);
        const QString known = resolveAppDisplayName(agg.processName, agg.samplePath);
        if (!known.isEmpty()) {
            lines << QStringLiteral("- %1 → %2").arg(agg.processName, known);
        } else if (!agg.samplePath.isEmpty()) {
            lines << QStringLiteral("- %1，路径: %2（请推断中文名）").arg(agg.processName, agg.samplePath);
        }
    }

    return lines.join(QStringLiteral("\n"));
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
    const QString thisWeekText = formatDuration(thisWeekSeconds);
    if (previousWeekSeconds <= 0) {
        return QStringLiteral("本周总时长 %1；上周无使用记录，无法计算环比。").arg(thisWeekText);
    }

    const qint64 diff = static_cast<qint64>(thisWeekSeconds) - previousWeekSeconds;
    const int pct = static_cast<int>((diff * 100) / previousWeekSeconds);
    const QString previousWeekText = formatDuration(previousWeekSeconds);

    QStringList parts;
    parts << QStringLiteral("本周总时长 %1").arg(thisWeekText);
    parts << QStringLiteral("上周总时长 %1").arg(previousWeekText);
    parts << QStringLiteral("时长差 %1").arg(formatDuration(static_cast<int>(qAbs(diff))));

    if (pct > 0) {
        parts << QStringLiteral("相比上周增加 %1%").arg(pct);
    } else if (pct < 0) {
        parts << QStringLiteral("相比上周减少 %1%").arg(-pct);
    } else {
        parts << QStringLiteral("与上周持平");
    }
    return parts.join(QStringLiteral("；"));
}
