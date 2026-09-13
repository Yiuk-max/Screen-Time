#include "startupmanager.h"

#include <QCoreApplication>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <appmodel.h>
#include <tlhelp32.h>

#ifdef SCREENTIME_USE_CPPWINRT
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#endif

#include <QDir>
#include <QSettings>
#include <QStringList>
#include <string>

#ifndef APPMODEL_ERROR_NO_PACKAGE
#define APPMODEL_ERROR_NO_PACKAGE 15700L
#endif

namespace {

constexpr wchar_t kStartupTaskId[] = L"ScreenTimeStartupTask";

const QString kHelperFlag = QStringLiteral("--winrt-helper");
constexpr DWORD kHelperTimeoutMs = 5000;
constexpr DWORD kHelperTimeoutExit = 0xFFFFFFFFu;

constexpr int kHelperOk = 0;
constexpr int kHelperError = 1;
constexpr int kHelperEnabled = 10;
constexpr int kHelperDisabled = 11;
constexpr int kHelperDisabledByUser = 12;
constexpr int kHelperDisabledByPolicy = 13;

bool isPackaged()
{
    UINT32 length = 0;
    return GetCurrentPackageFullName(&length, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
}

QSettings runKey()
{
    return QSettings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
}

bool runHelperProcess(const QString &operation, DWORD *exitCode)
{
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString commandLine =
        QStringLiteral("\"%1\" %2 %3").arg(exePath, kHelperFlag, operation);
    std::wstring command = commandLine.toStdWString();

    STARTUPINFOW startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));

    LPWSTR commandLineBuffer = command.empty() ? nullptr : &command[0];
    if (!CreateProcessW(nullptr, commandLineBuffer, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, nullptr, &startupInfo, &processInfo)) {
        return false;
    }

    DWORD code = kHelperTimeoutExit;
    if (WaitForSingleObject(processInfo.hProcess, kHelperTimeoutMs) == WAIT_OBJECT_0) {
        GetExitCodeProcess(processInfo.hProcess, &code);
    } else {
        TerminateProcess(processInfo.hProcess, kHelperTimeoutExit);
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    *exitCode = code;
    return true;
}

// MSIX StartupTask activation has no command-line marker and must not be
// inspected through WinRT in the main Qt process. Infer it from the parent
// process instead: interactive launches come from Explorer/Start Menu/terminals,
// while StartupTask is launched by a system service such as sihost or svchost.
DWORD parentProcessId()
{
    const DWORD self = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W entry;
    ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = sizeof(entry);

    DWORD parent = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == self) {
                parent = entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return parent;
}

QString processImageName(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return QString();
    }

    wchar_t buffer[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    QString name;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        name = QString::fromWCharArray(buffer);
        const int slash = name.lastIndexOf(QLatin1Char('\\'));
        if (slash >= 0) {
            name = name.mid(slash + 1);
        }
        name = name.toLower();
    }
    CloseHandle(process);
    return name;
}

bool parentLooksLikeUserLaunch()
{
    const DWORD parent = parentProcessId();
    if (!parent) {
        return false;
    }

    const QString name = processImageName(parent);
    static const QStringList shellParents = {
        QStringLiteral("explorer.exe"),
        QStringLiteral("cmd.exe"),
        QStringLiteral("powershell.exe"),
        QStringLiteral("pwsh.exe"),
        QStringLiteral("windowsterminal.exe"),
        QStringLiteral("wt.exe"),
        QStringLiteral("openconsole.exe"),
        QStringLiteral("conhost.exe"),
        QStringLiteral("devenv.exe"),
        QStringLiteral("code.exe"),
        QStringLiteral("startmenuexperiencehost.exe"),
    };
    return shellParents.contains(name);
}

} // namespace
#endif // Q_OS_WIN

bool StartupManager::isAutoStartEnabled()
{
#ifdef Q_OS_WIN
    if (isPackaged()) {
        DWORD code = kHelperTimeoutExit;
        if (runHelperProcess(QStringLiteral("query"), &code)) {
            if (code == kHelperEnabled) {
                return true;
            }
            if (code == kHelperDisabled || code == kHelperDisabledByUser ||
                code == kHelperDisabledByPolicy) {
                return false;
            }
        }
        // The manifest declares the task enabled, so assume enabled if the
        // isolated helper could not produce an answer.
        return true;
    }

    return runKey().contains(QStringLiteral("ScreenTime"));
#else
    return false;
#endif
}

bool StartupManager::setAutoStartEnabled(bool enabled)
{
#ifdef Q_OS_WIN
    if (isPackaged()) {
        DWORD code = kHelperTimeoutExit;
        if (!runHelperProcess(enabled ? QStringLiteral("enable") : QStringLiteral("disable"),
                              &code)) {
            return false;
        }
        return code == kHelperOk;
    }

    QSettings key = runKey();
    if (enabled) {
        const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        key.setValue(QStringLiteral("ScreenTime"),
                     QStringLiteral("\"%1\" --autostart").arg(appPath));
    } else {
        key.remove(QStringLiteral("ScreenTime"));
    }
    key.sync();
    return key.status() == QSettings::NoError;
#else
    Q_UNUSED(enabled);
    return false;
#endif
}

bool StartupManager::launchedByAutoStart()
{
    if (QCoreApplication::arguments().contains(QStringLiteral("--autostart"))) {
        return true;
    }

#ifdef Q_OS_WIN
    if (!isPackaged()) {
        return false;
    }
    return !parentLooksLikeUserLaunch();
#else
    return false;
#endif
}

int StartupManager::runWinRtHelper(const QString &operation)
{
#ifdef Q_OS_WIN
#ifdef SCREENTIME_USE_CPPWINRT
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        using winrt::Windows::ApplicationModel::StartupTask;
        using winrt::Windows::ApplicationModel::StartupTaskState;

        const auto task = StartupTask::GetAsync(kStartupTaskId).get();
        if (!task) {
            return kHelperError;
        }

        if (operation == QStringLiteral("query")) {
            switch (task.State()) {
            case StartupTaskState::Enabled:
            case StartupTaskState::EnabledByPolicy:
                return kHelperEnabled;
            case StartupTaskState::DisabledByUser:
                return kHelperDisabledByUser;
            case StartupTaskState::DisabledByPolicy:
                return kHelperDisabledByPolicy;
            default:
                return kHelperDisabled;
            }
        }

        if (operation == QStringLiteral("enable")) {
            const StartupTaskState state = task.State();
            if (state == StartupTaskState::Enabled || state == StartupTaskState::EnabledByPolicy) {
                return kHelperOk;
            }
            if (state == StartupTaskState::DisabledByUser ||
                state == StartupTaskState::DisabledByPolicy) {
                return kHelperError;
            }
            const StartupTaskState result = task.RequestEnableAsync().get();
            return (result == StartupTaskState::Enabled ||
                    result == StartupTaskState::EnabledByPolicy)
                       ? kHelperOk
                       : kHelperError;
        }

        if (operation == QStringLiteral("disable")) {
            task.Disable();
            return kHelperOk;
        }
    } catch (...) {
        return kHelperError;
    }
#endif
    Q_UNUSED(operation);
    return kHelperError;
#else
    Q_UNUSED(operation);
    return 0;
#endif
}
