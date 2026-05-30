#include "updater.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QApplication>
#include <QDebug>

#ifndef SCREENTIME_VERSION
#define SCREENTIME_VERSION "0.1.0"
#endif

Updater::Updater(QObject *parent)
    : QObject(parent)
{
}

QString Updater::currentVersion() const
{
    return QStringLiteral(SCREENTIME_VERSION);
}

QString Updater::latestVersion() const
{
    return m_latestVersion;
}

void Updater::checkForUpdates(bool silent)
{
    m_silentCheck = silent;

    const QString url = QStringLiteral("https://api.github.com/repos/Yiuk-max/Screen-Time/releases/latest");
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "ScreenTime-Updater");

    m_currentReply = m_networkManager.get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &Updater::onVersionCheckFinished);
}

void Updater::onVersionCheckFinished()
{
    if (!m_currentReply) {
        return;
    }

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit updateCheckFailed(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    const QByteArray data = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    m_latestVersion = parseLatestVersionFromJson(data);
    m_downloadUrl = parseDownloadUrlFromJson(data);
    m_releaseNotes = parseReleaseNotesFromJson(data);

    if (m_latestVersion.isEmpty() || m_downloadUrl.isEmpty()) {
        emit updateCheckFailed(QStringLiteral("无法解析版本信息"));
        return;
    }

    const QVersionNumber current = QVersionNumber::fromString(currentVersion());
    const QVersionNumber latest = QVersionNumber::fromString(m_latestVersion);

    if (latest > current) {
        emit updateAvailable(m_latestVersion, m_downloadUrl, m_releaseNotes);
    } else {
        if (!m_silentCheck) {
            emit noUpdateAvailable();
        }
    }
}

QString Updater::parseLatestVersionFromJson(const QByteArray &jsonData)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    const QJsonObject obj = doc.object();
    QString tag = obj.value(QStringLiteral("tag_name")).toString();
    if (tag.startsWith('v') || tag.startsWith('V')) {
        tag = tag.mid(1);
    }
    return tag;
}

QString Updater::parseDownloadUrlFromJson(const QByteArray &jsonData)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    const QJsonObject obj = doc.object();
    const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();

    for (const QJsonValue &asset : assets) {
        const QJsonObject assetObj = asset.toObject();
        const QString name = assetObj.value(QStringLiteral("name")).toString();
        if (name.endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
            return assetObj.value(QStringLiteral("browser_download_url")).toString();
        }
    }

    // 如果没有找到 zip，尝试使用 source zip
    return obj.value(QStringLiteral("zipball_url")).toString();
}

QString Updater::parseReleaseNotesFromJson(const QByteArray &jsonData)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError) {
        return QString();
    }

    const QJsonObject obj = doc.object();
    return obj.value(QStringLiteral("body")).toString();
}

void Updater::downloadAndInstallUpdate(const QString &downloadUrl)
{
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString fileName = QStringLiteral("ScreenTime_Update.zip");
    m_pendingDownloadPath = QDir(tempDir).absoluteFilePath(fileName);

    QNetworkRequest request{QUrl(downloadUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "ScreenTime-Updater");

    m_currentReply = m_networkManager.get(request);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &Updater::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished, this, &Updater::onDownloadFinished);
}

void Updater::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void Updater::onDownloadFinished()
{
    if (!m_currentReply) {
        return;
    }

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit downloadFailed(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QFile file(m_pendingDownloadPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit downloadFailed(QStringLiteral("无法创建文件: %1").arg(m_pendingDownloadPath));
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    file.write(m_currentReply->readAll());
    file.close();

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    emit downloadFinished(m_pendingDownloadPath);
    emit installUpdateRequested(m_pendingDownloadPath);
}
