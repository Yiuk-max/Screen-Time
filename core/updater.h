#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>

class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(QObject *parent = nullptr);

    void checkForUpdates(bool silent = false);
    void downloadAndInstallUpdate(const QString &downloadUrl);

    QString currentVersion() const;
    QString latestVersion() const;

signals:
    void updateAvailable(const QString &latestVersion, const QString &downloadUrl, const QString &releaseNotes);
    void noUpdateAvailable();
    void updateCheckFailed(const QString &error);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &filePath);
    void downloadFailed(const QString &error);
    void installUpdateRequested(const QString &zipFilePath);

private slots:
    void onVersionCheckFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QString parseLatestVersionFromJson(const QByteArray &jsonData);
    QString parseDownloadUrlFromJson(const QByteArray &jsonData);
    QString parseReleaseNotesFromJson(const QByteArray &jsonData);
    QString getAssetDownloadUrl(QNetworkReply *reply);

    QNetworkAccessManager m_networkManager;
    QNetworkReply *m_currentReply = nullptr;
    QString m_latestVersion;
    QString m_downloadUrl;
    QString m_releaseNotes;
    bool m_silentCheck = false;
    QString m_pendingDownloadPath;
};

#endif // UPDATER_H
