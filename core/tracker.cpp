#include "tracker.h"

#include <QDate>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

Tracker::Tracker(Database *database, QObject *parent)
    : QObject(parent)
    , m_database(database)
{
    m_tickTimer.setInterval(5000);
    connect(&m_tickTimer, &QTimer::timeout, this, &Tracker::onTick);
}

void Tracker::start()
{
    if (!m_tickTimer.isActive()) {
        m_tickTimer.start();
    }
}

void Tracker::stop()
{
    if (m_tickTimer.isActive()) {
        m_tickTimer.stop();
    }
    flushToDatabase();
}

void Tracker::onTick()
{
    const QString appPath = currentAppPath();
    const QString appName = currentAppName();
    const QString title = currentWindowTitle();

    if (m_lastAppName.isEmpty()) {
        m_lastAppName = appName;
        m_lastAppPath = appPath;
        m_lastWindowTitle = title;
    }

    if (appName == m_lastAppName) {
        m_accumulatedSeconds += 5;
    } else {
        flushToDatabase();
        m_lastAppName = appName;
        m_lastAppPath = appPath;
        m_lastWindowTitle = title;
        m_accumulatedSeconds = 5;
    }

    m_secondsSinceLastFlush += 5;
    if (m_secondsSinceLastFlush >= 30) {
        flushToDatabase();
    }
}

void Tracker::flushToDatabase()
{
    if (!m_database || m_lastAppName.isEmpty() || m_accumulatedSeconds <= 0) {
        return;
    }

    const QString date = QDate::currentDate().toString("yyyy-MM-dd");
    const QString trackedAt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_database->addRecord(date, m_lastAppName, m_lastWindowTitle, m_lastAppPath, m_accumulatedSeconds, trackedAt);

    m_accumulatedSeconds = 0;
    m_secondsSinceLastFlush = 0;
}

QString Tracker::currentAppName() const
{
    const QString fullPath = currentAppPath();
    if (fullPath.isEmpty()) {
        return QStringLiteral("UnknownApp");
    }
    const int slashPos = qMax(fullPath.lastIndexOf('/'), fullPath.lastIndexOf('\\'));
    if (slashPos >= 0 && slashPos + 1 < fullPath.size()) {
        return fullPath.mid(slashPos + 1);
    }
    return fullPath;
}

QString Tracker::currentAppPath() const
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        return QString();
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) {
        return QString();
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!process) {
        return QString();
    }

    wchar_t pathBuffer[MAX_PATH] = {0};
    const DWORD size = GetModuleFileNameExW(process, nullptr, pathBuffer, MAX_PATH);
    CloseHandle(process);

    if (size == 0) {
        return QString();
    }

    return QString::fromWCharArray(pathBuffer);
#else
    return QString();
#endif
}

QString Tracker::currentWindowTitle() const
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        return QStringLiteral("UnknownWindow");
    }

    wchar_t titleBuffer[512] = {0};
    const int length = GetWindowTextW(hwnd, titleBuffer, 512);
    if (length <= 0) {
        return QStringLiteral("UnknownWindow");
    }
    return QString::fromWCharArray(titleBuffer, length);
#else
    return QStringLiteral("UnsupportedOS");
#endif
}
