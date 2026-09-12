#include "startupmanager.h"

#include <QtGlobal>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <appmodel.h>
#include <roapi.h>
#include <winstring.h>
#include <inspectable.h>
#include <tlhelp32.h>

#ifdef SCREENTIME_USE_CPPWINRT
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#endif

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStringList>
#include <cwchar>
#include <string>

#ifndef APPMODEL_ERROR_NO_PACKAGE
#define APPMODEL_ERROR_NO_PACKAGE 15700L
#endif

namespace {

// 直接写文件的 WinRT 日志（qInfo 之后的日志可能被优化掉，这里用文件写入
// 的副作用保证日志一定落盘）。
void winrtLog(const QString &message)
{
    const QString path = QDir::homePath() + QStringLiteral("/ScreenTime_winrt.log");
    QFile file(path);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        const QString line =
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
            + QLatin1Char(' ') + message + QLatin1Char('\n');
        file.write(line.toUtf8());
    }
}

// ---------------------------------------------------------------------------
// 最小化声明的 WinRT 接口。
//
// 正常构建会让本文件单独使用 C++20 和 Windows SDK 的 C++/WinRT。
// 以下最小 ABI 声明只用于构建机未安装 C++/WinRT 头文件时的回退路径；
// 所有调用仍隔离在 helper 子进程中，回退 ABI 即使异常也不会拖垮主程序。
// 接口布局与 IID 取自 Windows SDK 的 winrt\windows.applicationmodel*.h。
// ---------------------------------------------------------------------------

// Windows.ApplicationModel.Activation.ActivationKind
constexpr INT32 kActivationKindStartupTask = 1020;

// Windows.Foundation.AsyncStatus
constexpr INT32 kAsyncStatusStarted = 0;
constexpr INT32 kAsyncStatusCompleted = 1;

// Windows.ApplicationModel.StartupTaskState
constexpr INT32 kStartupTaskDisabled = 0;
constexpr INT32 kStartupTaskDisabledByUser = 1;
constexpr INT32 kStartupTaskEnabled = 2;
constexpr INT32 kStartupTaskDisabledByPolicy = 3;
constexpr INT32 kStartupTaskEnabledByPolicy = 4;

// 运行库类名与任务 Id。
constexpr wchar_t kStartupTaskClass[] = L"Windows.ApplicationModel.StartupTask";
constexpr wchar_t kAppInstanceClass[] = L"Windows.ApplicationModel.AppInstance";
constexpr wchar_t kStartupTaskId[] = L"ScreenTimeStartupTask";

// IID_IStartupTaskStatics   {ee5b60bd-a148-41a7-b26e-e8b88a1e62f8}
const GUID kIIDStartupTaskStatics =
    {0xee5b60bd, 0xa148, 0x41a7, {0xb2, 0x6e, 0xe8, 0xb8, 0x8a, 0x1e, 0x62, 0xf8}};
// IID_IAppInstanceStatics   {9d11e77f-9ea6-47af-a6ec-46784c5ba254}
const GUID kIIDAppInstanceStatics =
    {0x9d11e77f, 0x9ea6, 0x47af, {0xa6, 0xec, 0x46, 0x78, 0x4c, 0x5b, 0xa2, 0x54}};
// IID_IStartupTaskActivatedEventArgs   {03b11a58-5276-4d91-8621-54611864d5fa}
const GUID kIIDStartupTaskActivatedEventArgs =
    {0x03b11a58, 0x5276, 0x4d91, {0x86, 0x21, 0x54, 0x61, 0x18, 0x64, 0xd5, 0xfa}};
// IID_IAsyncInfo   {00000036-0000-0000-c000-000000000046}
const GUID kIIDAsyncInfo =
    {0x00000036, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// ── 辅助进程的命令行 / 退出码 ─────────────────────────────────
// 所有 WinRT 调用都放到一个独立的辅助进程里做：这些 Windows.ApplicationModel
// 激活类 API 在 MSIX 全信任进程里一旦出错会直接崩溃（实测 0xC0000005 /
// DEP，执行到非法地址），跑在子进程里可以保证主程序永远能打开。
const wchar_t kHelperFlag[] = L"--winrt-helper";
const QString kHelperFlagQString = QStringLiteral("--winrt-helper");

constexpr int kHelperOk = 0;                 // 通用成功 / 不是 startupTask 激活
constexpr int kHelperError = 1;              // 失败
constexpr int kHelperEnabled = 10;           // 自启动已开启
constexpr int kHelperDisabled = 11;          // 自启动已关闭
constexpr int kHelperDisabledByUser = 12;    // 被用户禁用
constexpr int kHelperDisabledByPolicy = 13;  // 被策略禁用
constexpr int kHelperStartupTask = 20;       // 本次由 startupTask 拉起

constexpr DWORD kHelperTimeoutMs = 3000;
constexpr DWORD kHelperTimeoutExit = 0xFFFFFFFFu;

struct IStartupTask;

// Windows.Foundation.IAsyncOperation`1 的 ABI vtable。
//
// 关键：参数化接口 IAsyncOperation<T> 在 ABI 里被“扁平化”，vtable 只有
//   IInspectable + put_Completed + get_Completed + GetResults
// 三个方法；IAsyncInfo（get_Status 等）并不在它的 vtable 里，必须
// QueryInterface(IID_IAsyncInfo) 拿到单独的接口再查询状态。
// 早期版本把 IAsyncInfo 的 5 个方法直接塞进了这个 vtable，导致
// GetResults 在错误槽位上调用 → 0xC0000005（程序直接打不开）。
struct IAsyncOperationBase : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE put_Completed(IInspectable *handler) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Completed(IInspectable **handler) = 0;
};

struct IAsyncOperationStartupTask : public IAsyncOperationBase
{
    virtual HRESULT STDMETHODCALLTYPE GetResults(IStartupTask **result) = 0;
};

struct IAsyncOperationStartupTaskState : public IAsyncOperationBase
{
    virtual HRESULT STDMETHODCALLTYPE GetResults(INT32 *result) = 0;
};

// Windows.Foundation.IAsyncInfo（通过 QI 获取）
struct IAsyncInfo : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE get_Id(UINT32 *id) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Status(INT32 *status) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ErrorCode(HRESULT *errorCode) = 0;
    virtual HRESULT STDMETHODCALLTYPE Cancel() = 0;
    virtual HRESULT STDMETHODCALLTYPE Close() = 0;
};

// Windows.ApplicationModel.IStartupTask
struct IStartupTask : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE RequestEnableAsync(IAsyncOperationStartupTaskState **operation) = 0;
    virtual HRESULT STDMETHODCALLTYPE Disable() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_State(INT32 *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_TaskId(HSTRING *value) = 0;
};

// Windows.ApplicationModel.IStartupTaskStatics
struct IStartupTaskStatics : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE GetForCurrentPackageAsync(IInspectable **operation) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAsync(HSTRING taskId,
                                               IAsyncOperationStartupTask **operation) = 0;
};

// Windows.ApplicationModel.Activation.IActivatedEventArgs
struct IActivatedEventArgs : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE get_Kind(INT32 *value) = 0;
};

// Windows.ApplicationModel.Activation.IStartupTaskActivatedEventArgs
// （默认接口，同时实现 IActivatedEventArgs）
struct IStartupTaskActivatedEventArgs : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE get_TaskId(HSTRING *value) = 0;
};

// Windows.ApplicationModel.IAppInstanceStatics
struct IAppInstanceStatics : public IInspectable
{
    virtual HRESULT STDMETHODCALLTYPE get_RecommendedInstance(IInspectable **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetActivatedEventArgs(IActivatedEventArgs **result) = 0;
};

HSTRING makeHString(const wchar_t *text)
{
    if (!text) {
        return nullptr;
    }
    HSTRING value = nullptr;
    const UINT32 length = static_cast<UINT32>(std::wcslen(text));
    if (FAILED(WindowsCreateString(text, length, &value))) {
        return nullptr;
    }
    return value;
}

// 轮询等待异步操作完成。
// IAsyncInfo 不在 IAsyncOperation 的 vtable 里，需要先 QI。
bool awaitAsync(IAsyncOperationBase *operation)
{
    if (!operation) {
        return false;
    }

    IAsyncInfo *info = nullptr;
    if (FAILED(operation->QueryInterface(kIIDAsyncInfo, reinterpret_cast<void **>(&info))) || !info) {
        winrtLog(QStringLiteral("await: QueryInterface(IAsyncInfo) failed"));
        return false;
    }

    INT32 status = kAsyncStatusStarted;
    for (int i = 0; i < 1000; ++i) {
        if (FAILED(info->get_Status(&status)) || status != kAsyncStatusStarted) {
            break;
        }
        ::Sleep(10);
    }
    info->Release();
    return status == kAsyncStatusCompleted;
}

// 获取当前包声明的 startupTask 对象，失败返回 nullptr。
// 只能在辅助进程里调用（见 runWinRtHelper）。
IStartupTask *acquireStartupTask()
{
    HSTRING classId = makeHString(kStartupTaskClass);
    if (!classId) {
        winrtLog(QStringLiteral("acquire: makeHString(StartupTask) failed"));
        return nullptr;
    }
    IStartupTaskStatics *statics = nullptr;
    const HRESULT hr = RoGetActivationFactory(classId, kIIDStartupTaskStatics,
                                              reinterpret_cast<void **>(&statics));
    WindowsDeleteString(classId);
    if (FAILED(hr) || !statics) {
        winrtLog(QStringLiteral("acquire: StartupTask factory hr=0x%1")
                     .arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0')));
        return nullptr;
    }

    HSTRING taskId = makeHString(kStartupTaskId);
    if (!taskId) {
        statics->Release();
        return nullptr;
    }
    IAsyncOperationStartupTask *operation = nullptr;
    const HRESULT getHr = statics->GetAsync(taskId, &operation);
    WindowsDeleteString(taskId);
    statics->Release();
    if (FAILED(getHr) || !operation) {
        winrtLog(QStringLiteral("acquire: GetAsync hr=0x%1")
                     .arg(static_cast<quint32>(getHr), 8, 16, QLatin1Char('0')));
        return nullptr;
    }

    IStartupTask *task = nullptr;
    if (awaitAsync(operation)) {
        operation->GetResults(&task);
    }
    operation->Release();
    if (!task) {
        winrtLog(QStringLiteral("acquire: no task (task id mismatch?)"));
    }
    return task;
}

bool isStartupStateEnabled(INT32 state)
{
    return state == kStartupTaskEnabled || state == kStartupTaskEnabledByPolicy;
}

// 请求开启 startupTask。用户/策略禁用时返回 false。
bool requestEnableStartupTask(IStartupTask *task)
{
    if (!task) {
        return false;
    }
    INT32 state = kStartupTaskDisabled;
    if (FAILED(task->get_State(&state))) {
        return false;
    }
    if (isStartupStateEnabled(state)) {
        return true;
    }
    if (state == kStartupTaskDisabledByUser || state == kStartupTaskDisabledByPolicy) {
        winrtLog(QStringLiteral("startup: cannot enable, blocked by user/policy (state=%1)")
                     .arg(state));
        return false;
    }

    IAsyncOperationStartupTaskState *operation = nullptr;
    if (FAILED(task->RequestEnableAsync(&operation)) || !operation) {
        winrtLog(QStringLiteral("startup: RequestEnableAsync failed"));
        return false;
    }
    INT32 result = kStartupTaskDisabled;
    bool success = false;
    if (awaitAsync(operation) && SUCCEEDED(operation->GetResults(&result))) {
        success = isStartupStateEnabled(result);
    }
    operation->Release();
    winrtLog(QStringLiteral("startup: enable success=%1 state=%2")
                 .arg(success ? 1 : 0)
                 .arg(result));
    return success;
}

// 本次进程是否由 startupTask 拉起。
bool detectStartupTaskActivation()
{
    HSTRING classId = makeHString(kAppInstanceClass);
    if (!classId) {
        return false;
    }
    IAppInstanceStatics *statics = nullptr;
    const HRESULT hr = RoGetActivationFactory(classId, kIIDAppInstanceStatics,
                                              reinterpret_cast<void **>(&statics));
    WindowsDeleteString(classId);
    if (FAILED(hr) || !statics) {
        winrtLog(QStringLiteral("launch: AppInstance factory hr=0x%1")
                     .arg(static_cast<quint32>(hr), 8, 16, QLatin1Char('0')));
        return false;
    }

    IActivatedEventArgs *eventArgs = nullptr;
    const HRESULT getHr = statics->GetActivatedEventArgs(&eventArgs);
    statics->Release();
    if (FAILED(getHr) || !eventArgs) {
        winrtLog(QStringLiteral("launch: GetActivatedEventArgs hr=0x%1 null=%2")
                     .arg(static_cast<quint32>(getHr), 8, 16, QLatin1Char('0'))
                     .arg(eventArgs ? 0 : 1));
        return false;
    }

    bool isStartupTask = false;
    // 首选：直接问默认接口要 TaskId，最可靠。
    IStartupTaskActivatedEventArgs *startupArgs = nullptr;
    if (SUCCEEDED(eventArgs->QueryInterface(kIIDStartupTaskActivatedEventArgs,
                                            reinterpret_cast<void **>(&startupArgs)))
        && startupArgs) {
        HSTRING taskId = nullptr;
        if (SUCCEEDED(startupArgs->get_TaskId(&taskId)) && taskId) {
            const wchar_t *raw = WindowsGetStringRawBuffer(taskId, nullptr);
            isStartupTask = raw && std::wcscmp(raw, kStartupTaskId) == 0;
            WindowsDeleteString(taskId);
        }
        startupArgs->Release();
    }

    INT32 kind = 0;
    const HRESULT kindHr = eventArgs->get_Kind(&kind);
    eventArgs->Release();
    if (!isStartupTask && SUCCEEDED(kindHr)) {
        isStartupTask = (kind == kActivationKindStartupTask);
    }
    winrtLog(QStringLiteral("launch: kind=%1 (StartupTask=%2) isStartupTask=%3")
                 .arg(kind)
                 .arg(kActivationKindStartupTask)
                 .arg(isStartupTask ? 1 : 0));
    return isStartupTask;
}

QSettings runKey()
{
    return QSettings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
}

// ── 父进程回退判断 ────────────────────────────────────────────
// 若 AppInstance 激活查询不可用（某些系统上会直接崩溃），用父进程名判断：
// 用户从桌面/开始菜单/终端启动时，父进程一般是 explorer / 终端；
// 被 startupTask 拉起时父进程是系统服务（svchost / RuntimeBroker 等）。
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
    winrtLog(QStringLiteral("launch: parent=%1 user=%2")
                 .arg(name.isEmpty() ? QStringLiteral("?") : name)
                 .arg(shellParents.contains(name) ? 1 : 0));
    return shellParents.contains(name);
}

// 清理历史版本留下的「WinRT 已永久禁用」标记。
// 旧实现一旦某次在 WinRT 里崩溃，就会把这些标记写进设置并从此
// 不再调用自启动相关 API，导致开关和托盘启动永久失效。
void clearLegacyWinRtFlags()
{
    static const bool cleaned = []() {
        QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
        const QStringList keys = {
            QStringLiteral("startup/winrt_disabled"),
            QStringLiteral("startup/winrt_disabled_startup"),
            QStringLiteral("startup/winrt_disabled_launch"),
            QStringLiteral("startup/winrt_probe"),
            QStringLiteral("startup/winrt_probe_running"),
            QStringLiteral("startup/winrt_probe_startup"),
            QStringLiteral("startup/winrt_probe_launch"),
        };
        bool changed = false;
        for (const QString &key : keys) {
            if (s.contains(key)) {
                s.remove(key);
                changed = true;
            }
        }
        if (changed) {
            s.sync();
            winrtLog(QStringLiteral("cleared legacy winrt disable flags"));
        }
        return true;
    }();
    Q_UNUSED(cleaned);
}

// 启动辅助进程执行一次 WinRT 操作，返回退出码。
// 子进程崩溃/卡死都不影响主程序。
bool runHelperProcess(const QString &operation, DWORD *exitCode)
{
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QString commandLine = QStringLiteral("\"%1\" %2 %3")
                              .arg(exePath, kHelperFlagQString, operation);
    std::wstring command = commandLine.toStdWString();

    STARTUPINFOW startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));

    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo)) {
        winrtLog(QStringLiteral("helper: CreateProcess(%1) failed err=%2")
                     .arg(operation)
                     .arg(GetLastError()));
        return false;
    }

    DWORD code = kHelperTimeoutExit;
    const DWORD wait = WaitForSingleObject(processInfo.hProcess, kHelperTimeoutMs);
    if (wait == WAIT_OBJECT_0) {
        GetExitCodeProcess(processInfo.hProcess, &code);
    } else {
        winrtLog(QStringLiteral("helper: %1 timed out, terminating").arg(operation));
        TerminateProcess(processInfo.hProcess, kHelperTimeoutExit);
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    winrtLog(QStringLiteral("helper: %1 exit=0x%2")
                 .arg(operation)
                 .arg(static_cast<quint32>(code), 8, 16, QLatin1Char('0')));
    *exitCode = code;
    return true;
}

} // namespace
#endif // Q_OS_WIN

bool StartupManager::isPackaged()
{
#ifdef Q_OS_WIN
    UINT32 length = 0;
    const LONG result = GetCurrentPackageFullName(&length, nullptr);
    const bool packaged = result != APPMODEL_ERROR_NO_PACKAGE;
    if (packaged) {
        clearLegacyWinRtFlags();
    }
    return packaged;
#else
    return false;
#endif
}

const char *StartupManager::startupTaskId()
{
    return "ScreenTimeStartupTask";
}

bool StartupManager::isAutoStartSupported()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

int StartupManager::runWinRtHelper(const QString &operation)
{
#ifdef Q_OS_WIN
    int result = kHelperError;

    {
        UINT32 length = 0;
        const bool packaged = GetCurrentPackageFullName(&length, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
        winrtLog(QStringLiteral("helper[%1]: packaged=%2 backend=%3")
                     .arg(operation)
                     .arg(packaged ? 1 : 0)
#ifdef SCREENTIME_USE_CPPWINRT
                     .arg(QStringLiteral("cppwinrt")));
#else
                     .arg(QStringLiteral("abi-fallback")));
#endif
    }

#ifdef SCREENTIME_USE_CPPWINRT
    // 使用 Windows SDK 生成的官方 ABI，不再依赖 MinGW 下手写的参数化
    // IAsyncOperation<T> 接口。Win10 上后者会在 GetAsync/GetResults 附近
    // 触发 0xC0000005，正是日志中 helper[query] 崩溃的来源。
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        using winrt::Windows::ApplicationModel::StartupTask;
        using winrt::Windows::ApplicationModel::StartupTaskState;

        if (operation == QStringLiteral("query")
            || operation == QStringLiteral("enable")
            || operation == QStringLiteral("disable")) {
            const auto task = StartupTask::GetAsync(kStartupTaskId).get();
            if (!task) {
                winrtLog(QStringLiteral("helper[%1]: StartupTask not found").arg(operation));
            } else if (operation == QStringLiteral("query")) {
                switch (task.State()) {
                case StartupTaskState::Enabled:
                case StartupTaskState::EnabledByPolicy:
                    result = kHelperEnabled;
                    break;
                case StartupTaskState::DisabledByUser:
                    result = kHelperDisabledByUser;
                    break;
                case StartupTaskState::DisabledByPolicy:
                    result = kHelperDisabledByPolicy;
                    break;
                default:
                    result = kHelperDisabled;
                    break;
                }
            } else if (operation == QStringLiteral("enable")) {
                const StartupTaskState state = task.State();
                if (state == StartupTaskState::Enabled
                    || state == StartupTaskState::EnabledByPolicy) {
                    result = kHelperOk;
                } else if (state != StartupTaskState::DisabledByUser
                           && state != StartupTaskState::DisabledByPolicy) {
                    const StartupTaskState enabledState = task.RequestEnableAsync().get();
                    result = (enabledState == StartupTaskState::Enabled
                              || enabledState == StartupTaskState::EnabledByPolicy)
                                 ? kHelperOk : kHelperError;
                }
            } else {
                task.Disable();
                result = kHelperOk;
            }
        } else {
            result = kHelperOk;
        }
    } catch (const winrt::hresult_error &error) {
        winrtLog(QStringLiteral("helper[%1]: WinRT hr=0x%2 message=%3")
                     .arg(operation)
                     .arg(static_cast<quint32>(error.code().value), 8, 16, QLatin1Char('0'))
                     .arg(QString::fromWCharArray(error.message().c_str())));
    } catch (...) {
        winrtLog(QStringLiteral("helper[%1]: unknown C++/WinRT exception").arg(operation));
    }
#else
    RoInitialize(RO_INIT_MULTITHREADED);
    if (operation == QStringLiteral("query")) {
        IStartupTask *task = acquireStartupTask();
        if (task) {
            INT32 state = kStartupTaskDisabled;
            if (SUCCEEDED(task->get_State(&state))) {
                switch (state) {
                case kStartupTaskEnabled:
                case kStartupTaskEnabledByPolicy:
                    result = kHelperEnabled;
                    break;
                case kStartupTaskDisabledByUser:
                    result = kHelperDisabledByUser;
                    break;
                case kStartupTaskDisabledByPolicy:
                    result = kHelperDisabledByPolicy;
                    break;
                default:
                    result = kHelperDisabled;
                    break;
                }
            }
            task->Release();
        }
    } else if (operation == QStringLiteral("enable")) {
        IStartupTask *task = acquireStartupTask();
        if (task) {
            result = requestEnableStartupTask(task) ? kHelperOk : kHelperError;
            task->Release();
        }
    } else if (operation == QStringLiteral("disable")) {
        IStartupTask *task = acquireStartupTask();
        if (task) {
            result = SUCCEEDED(task->Disable()) ? kHelperOk : kHelperError;
            task->Release();
        }
    } else {
        result = kHelperOk;
    }
#endif

    winrtLog(QStringLiteral("helper[%1]: result=%2").arg(operation).arg(result));
    return result;
#else
    Q_UNUSED(operation);
    return 0;
#endif
}

bool StartupManager::isAutoStartEnabled()
{
#ifdef Q_OS_WIN
    if (isPackaged()) {
        DWORD code = kHelperTimeoutExit;
        if (runHelperProcess(QStringLiteral("query"), &code)) {
            switch (code) {
            case kHelperEnabled:
                return true;
            case kHelperDisabled:
            case kHelperDisabledByUser:
            case kHelperDisabledByPolicy:
                return false;
            default:
                break; // 辅助进程崩溃/超时，下面按默认值处理
            }
        }
        // 查询不可用：清单里声明的是 Enabled="true"，安装后默认就是开启的。
        winrtLog(QStringLiteral("isAutoStartEnabled: query unavailable, assume enabled"));
        return true;
    }

    QSettings key = runKey();
    return key.contains(QStringLiteral("ScreenTime"));
#else
    return false;
#endif
}

bool StartupManager::setAutoStartEnabled(bool enabled)
{
#ifdef Q_OS_WIN
    if (isPackaged()) {
        DWORD code = kHelperTimeoutExit;
        if (runHelperProcess(enabled ? QStringLiteral("enable") : QStringLiteral("disable"), &code)) {
            return code == kHelperOk;
        }
        return false;
    }

    QSettings key = runKey();
    if (enabled) {
        const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        key.setValue(QStringLiteral("ScreenTime"), QStringLiteral("\"%1\" --autostart").arg(appPath));
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
    // 传统安装方式，以及启动器透传的参数。
    if (QCoreApplication::arguments().contains(QStringLiteral("--autostart"))) {
        return true;
    }

#ifdef Q_OS_WIN
    if (!isPackaged()) {
        return false;
    }

    // MSIX 下 startupTask 直接拉起 ScreenTime.exe，没有命令行参数。
    // Windows.ApplicationModel.AppInstance 在本环境里会直接崩溃，因此
    // 用父进程名判断：用户从桌面/开始菜单/终端启动 -> explorer/终端；
    // 被系统 startupTask 拉起 -> 系统服务（svchost 等）。
    return !parentLooksLikeUserLaunch();
#else
    return false;
#endif
}
