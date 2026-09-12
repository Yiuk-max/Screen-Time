#include "startupmanager.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
// windows.h must precede appmodel.h with MinGW.
#include <windows.h>
#include <appmodel.h>
#include <inspectable.h>
#include <roapi.h>
#include <winstring.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <cwchar>

#ifndef APPMODEL_ERROR_NO_PACKAGE
#define APPMODEL_ERROR_NO_PACKAGE 15700L
#endif

namespace {

// MinGW 构建只需以下少量 WinRT ABI，用于访问 MSIX StartupTask。

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
const GUID kIIDStartupTaskStatics = {
    0xee5b60bd,
    0xa148,
    0x41a7,
    {0xb2, 0x6e, 0xe8, 0xb8, 0x8a, 0x1e, 0x62, 0xf8}};
// IID_IAppInstanceStatics   {9d11e77f-9ea6-47af-a6ec-46784c5ba254}
const GUID kIIDAppInstanceStatics = {
    0x9d11e77f,
    0x9ea6,
    0x47af,
    {0xa6, 0xec, 0x46, 0x78, 0x4c, 0x5b, 0xa2, 0x54}};
const GUID kIIDAsyncInfo = {
    0x00000036,
    0x0000,
    0x0000,
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

struct IStartupTask;

struct IAsyncOperationBase : public IInspectable {
  virtual HRESULT STDMETHODCALLTYPE put_Completed(IInspectable *handler) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Completed(IInspectable **handler) = 0;
};

struct IAsyncOperationStartupTask : public IAsyncOperationBase {
  virtual HRESULT STDMETHODCALLTYPE GetResults(IStartupTask **result) = 0;
};

struct IAsyncOperationStartupTaskState : public IAsyncOperationBase {
  virtual HRESULT STDMETHODCALLTYPE GetResults(INT32 *result) = 0;
};

struct IAsyncInfo : public IInspectable {
  virtual HRESULT STDMETHODCALLTYPE get_Id(UINT32 *id) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_Status(INT32 *status) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_ErrorCode(HRESULT *errorCode) = 0;
  virtual HRESULT STDMETHODCALLTYPE Cancel() = 0;
  virtual HRESULT STDMETHODCALLTYPE Close() = 0;
};

// Windows.ApplicationModel.IStartupTask
struct IStartupTask : public IInspectable {
  virtual HRESULT STDMETHODCALLTYPE
  RequestEnableAsync(IAsyncOperationStartupTaskState **operation) = 0;
  virtual HRESULT STDMETHODCALLTYPE Disable() = 0;
  virtual HRESULT STDMETHODCALLTYPE get_State(INT32 *value) = 0;
  virtual HRESULT STDMETHODCALLTYPE get_TaskId(HSTRING *value) = 0;
};

// Windows.ApplicationModel.IStartupTaskStatics
struct IStartupTaskStatics : public IInspectable {
  virtual HRESULT STDMETHODCALLTYPE
  GetForCurrentPackageAsync(IInspectable **operation) = 0;
  virtual HRESULT STDMETHODCALLTYPE
  GetAsync(HSTRING taskId, IAsyncOperationStartupTask **operation) = 0;
};

// Windows.ApplicationModel.Activation.IActivatedEventArgs
struct IActivatedEventArgs : public IInspectable {
  virtual HRESULT STDMETHODCALLTYPE get_Kind(INT32 *value) = 0;
};

// Windows.ApplicationModel.IAppInstanceStatics
struct IAppInstanceStatics : public IInspectable {
  virtual HRESULT STDMETHODCALLTYPE
  get_RecommendedInstance(IInspectable **value) = 0;
  virtual HRESULT STDMETHODCALLTYPE
  GetActivatedEventArgs(IActivatedEventArgs **result) = 0;
};

bool ensureWinRt() {
  static const bool initialized = []() {
    const HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
  }();
  return initialized;
}

HSTRING makeHString(const wchar_t *text) {
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

bool awaitAsync(IAsyncOperationBase *operation) {
  if (!operation) {
    return false;
  }

  IAsyncInfo *info = nullptr;
  if (FAILED(operation->QueryInterface(kIIDAsyncInfo,
                                       reinterpret_cast<void **>(&info))) ||
      !info) {
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

IStartupTask *acquireStartupTask() {
  if (!ensureWinRt()) {
    return nullptr;
  }

  HSTRING classId = makeHString(kStartupTaskClass);
  if (!classId) {
    return nullptr;
  }
  IStartupTaskStatics *statics = nullptr;
  const HRESULT hr = RoGetActivationFactory(
      classId, kIIDStartupTaskStatics, reinterpret_cast<void **>(&statics));
  WindowsDeleteString(classId);
  if (FAILED(hr) || !statics) {
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
    return nullptr;
  }

  IStartupTask *task = nullptr;
  if (awaitAsync(operation)) {
    operation->GetResults(&task);
  }
  operation->Release();
  return task;
}

bool isStartupStateEnabled(INT32 state) {
  return state == kStartupTaskEnabled || state == kStartupTaskEnabledByPolicy;
}

QSettings runKey() {
  return QSettings(
      QStringLiteral("HKEY_CURRENT_"
                     "USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
      QSettings::NativeFormat);
}

bool isPackaged() {
  UINT32 length = 0;
  return GetCurrentPackageFullName(&length, nullptr) !=
         APPMODEL_ERROR_NO_PACKAGE;
}

} // namespace
#endif // Q_OS_WIN

bool StartupManager::isAutoStartEnabled() {
#ifdef Q_OS_WIN
  if (isPackaged()) {
    IStartupTask *task = acquireStartupTask();
    if (!task) {
      return false;
    }
    INT32 state = kStartupTaskDisabled;
    task->get_State(&state);
    task->Release();
    return isStartupStateEnabled(state);
  }

  QSettings key = runKey();
  return key.contains(QStringLiteral("ScreenTime"));
#else
  return false;
#endif
}

bool StartupManager::setAutoStartEnabled(bool enabled) {
#ifdef Q_OS_WIN
  if (isPackaged()) {
    IStartupTask *task = acquireStartupTask();
    if (!task) {
      return false;
    }

    bool success = false;
    if (enabled) {
      INT32 state = kStartupTaskDisabled;
      task->get_State(&state);
      if (isStartupStateEnabled(state)) {
        success = true;
      } else if (state == kStartupTaskDisabledByUser ||
                 state == kStartupTaskDisabledByPolicy) {
        // 已被用户在“任务管理器 -> 启动”或组策略禁用，程序无法直接改回。
        success = false;
      } else {
        IAsyncOperationStartupTaskState *operation = nullptr;
        if (SUCCEEDED(task->RequestEnableAsync(&operation)) && operation) {
          INT32 result = kStartupTaskDisabled;
          if (awaitAsync(operation) &&
              SUCCEEDED(operation->GetResults(&result))) {
            success = isStartupStateEnabled(result);
          }
          operation->Release();
        }
      }
    } else {
      success = SUCCEEDED(task->Disable());
    }
    task->Release();
    return success;
  }

  QSettings key = runKey();
  if (enabled) {
    const QString appPath =
        QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
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

bool StartupManager::launchedByAutoStart() {
  if (QCoreApplication::arguments().contains(QStringLiteral("--autostart"))) {
    return true;
  }

#ifdef Q_OS_WIN
  if (!isPackaged() || !ensureWinRt()) {
    return false;
  }

  HSTRING classId = makeHString(kAppInstanceClass);
  if (!classId) {
    return false;
  }
  IAppInstanceStatics *statics = nullptr;
  const HRESULT hr = RoGetActivationFactory(
      classId, kIIDAppInstanceStatics, reinterpret_cast<void **>(&statics));
  WindowsDeleteString(classId);
  if (FAILED(hr) || !statics) {
    return false;
  }

  IActivatedEventArgs *eventArgs = nullptr;
  const HRESULT getHr = statics->GetActivatedEventArgs(&eventArgs);
  statics->Release();
  if (FAILED(getHr) || !eventArgs) {
    return false;
  }

  INT32 kind = 0;
  const HRESULT kindHr = eventArgs->get_Kind(&kind);
  eventArgs->Release();
  if (FAILED(kindHr)) {
    return false;
  }
  return kind == kActivationKindStartupTask;
#else
  return false;
#endif
}
