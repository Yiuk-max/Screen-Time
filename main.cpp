#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTimer>

#include "core/database.h"
#include "core/startupmanager.h"
#include "core/tracker.h"
#include "ui/mainwindow.h"
#include "ui/theme/thememanager.h"
#include "ui/i18n/translationmanager.h"

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

namespace {

// ── 启动诊断日志 ─────────────────────────────────────────────────
// 同时写多个位置，方便在 MSIX 沙箱里找到：用户主目录、桌面、应用数据目录。
QStringList logPaths()
{
    QStringList paths;
    const QString home = QDir::homePath();
    if (!home.isEmpty()) {
        paths << home + QStringLiteral("/ScreenTime_startup.log");
    }
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (!desktop.isEmpty()) {
        paths << desktop + QStringLiteral("/ScreenTime_startup.log");
    }
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!appData.isEmpty()) {
        QDir().mkpath(appData);
        paths << appData + QStringLiteral("/startup.log");
    }
    return paths;
}

void writeAll(const QByteArray &data, QIODevice::OpenMode mode)
{
    for (const QString &path : logPaths()) {
        QFile file(path);
        if (file.open(mode)) {
            file.write(data);
        }
    }
}

void logLine(const QString &message)
{
    const QString line =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
        + QLatin1Char(' ') + message + QLatin1Char('\n');
    writeAll(line.toUtf8(), QIODevice::Append | QIODevice::Text);
}

void resetLog()
{
    const QByteArray header =
        QByteArray("===== Screen Time startup log =====\n");
    writeAll(header, QIODevice::WriteOnly | QIODevice::Text);
}

void messageHandler(QtMsgType, const QMessageLogContext &, const QString &message)
{
    logLine(QStringLiteral("[qt] ") + message);
}

#ifdef Q_OS_WIN
LONG WINAPI crashHandler(EXCEPTION_POINTERS *info)
{
    const quint32 code = (info && info->ExceptionRecord) ? info->ExceptionRecord->ExceptionCode : 0;
    const quintptr address =
        (info && info->ExceptionRecord)
            ? reinterpret_cast<quintptr>(info->ExceptionRecord->ExceptionAddress) : 0;

    QString module = QStringLiteral("?");
    HMODULE mod = nullptr;
    if (address
        && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                  | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(address), &mod)
        && mod) {
        wchar_t path[MAX_PATH] = {0};
        if (GetModuleFileNameW(mod, path, MAX_PATH) > 0) {
            const quintptr base = reinterpret_cast<quintptr>(mod);
            module = QStringLiteral("%1+0x%2")
                         .arg(QString::fromWCharArray(path))
                         .arg(address - base, 0, 16);
        }
    }

    QString access;
    if (info && info->ExceptionRecord && info->ExceptionRecord->NumberParameters >= 2) {
        access = QStringLiteral(" access=%1 target=0x%2")
                     .arg(info->ExceptionRecord->ExceptionInformation[0])
                     .arg(info->ExceptionRecord->ExceptionInformation[1], 0, 16);
    }

    logLine(QStringLiteral("!! CRASH code=0x%1 address=0x%2 module=%3%4")
                .arg(code, 8, 16, QLatin1Char('0'))
                .arg(address, 0, 16)
                .arg(module, access));
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 辅助进程模式：只执行一次 WinRT 操作就退出，不创建任何 Qt 对象。
    // 这些 Windows.ApplicationModel 激活类 API 在 MSIX 全信任进程里出错时
    // 会直接崩溃，放到子进程里跑可保证主程序永远能打开。
    {
        const QString commandLine = QString::fromWCharArray(GetCommandLineW());
        const QString flag = QStringLiteral("--winrt-helper");
        const int pos = commandLine.indexOf(flag);
        if (pos >= 0) {
            const QString rest = commandLine.mid(pos + flag.size()).trimmed();
            const int space = rest.indexOf(QLatin1Char(' '));
            const QString operation = (space >= 0 ? rest.left(space) : rest).trimmed();
            return StartupManager::runWinRtHelper(operation);
        }
    }
#endif
    QApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(crashHandler);
#endif

    resetLog();
    logLine(QStringLiteral("start; args=[%1]; exe=%2")
                .arg(QCoreApplication::arguments().join(QLatin1Char(' ')),
                     QCoreApplication::applicationFilePath()));

    app.setOrganizationName(QStringLiteral("ScreenTime"));
    app.setApplicationName(QStringLiteral("ScreenTime"));
    app.setQuitOnLastWindowClosed(false); // 关键：关闭窗口不退出程序

    // 必须在数据库和主窗口等耗时初始化之前捕获启动来源。MSIX startupTask
    // 的 sihost/系统父进程可能很快退出，太晚查询会丢失父进程信息。
    const bool launchedByAutoStart = StartupManager::launchedByAutoStart();
    app.setProperty("launchedByAutoStart", launchedByAutoStart);
    logLine(QStringLiteral("launch origin captured early: autoStart=%1")
                .arg(launchedByAutoStart));

    // 主题与界面完全解耦：全局样式 / 调色板由 ThemeManager 统一提供，
    // 主题切换通过 themeChanged 信号通知各控件自行刷新。
    ThemeManager::instance().applyTo(&app);
    // 初始化翻译（读取上次选择的界面语言）。
    TranslationManager::instance();
    logLine(QStringLiteral("theme + i18n ready"));

    Database database;
    database.init();
    logLine(QStringLiteral("database ready"));
    Tracker tracker(&database);
    tracker.start();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &tracker, &Tracker::stop);
    logLine(QStringLiteral("tracker started"));

    MainWindow window(&database);
    logLine(QStringLiteral("main window constructed"));

    // 兼容两种自启动方式：传统 exe 的 --autostart 参数，以及 MSIX
    // startupTask（后者没有命令行参数，启动来源已在初始化最前面捕获）。
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const QString launchMode =
        settings.value(QStringLiteral("startup/launch_mode"), QStringLiteral("tray")).toString();
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    logLine(QStringLiteral("launchedByAutoStart=%1 launchMode=%2 trayAvailableInitially=%3")
                .arg(launchedByAutoStart).arg(launchMode).arg(trayAvailable));

    // Windows 10 登录早期，startupTask 可能早于 Explorer 的 Shell_TrayWnd
    // 启动，此时 isSystemTrayAvailable() 会短暂返回 false。不能因此显示主窗；
    // MainWindow 会在后台重试注册托盘图标。若两分钟后托盘仍不可用，再显示
    // 主窗作为安全回退，避免产生用户无法操作的隐藏进程。
    if (launchedByAutoStart && launchMode == QStringLiteral("tray")) {
        window.hide();
        logLine(QStringLiteral("window hidden for autostart tray mode"));
        if (!trayAvailable) {
            QTimer::singleShot(120000, &window, [&window]() {
                if (!QSystemTrayIcon::isSystemTrayAvailable()) {
                    window.show();
                    window.raise();
                    window.activateWindow();
                    logLine(QStringLiteral("tray unavailable after 120s; window shown as fallback"));
                } else {
                    logLine(QStringLiteral("tray became available; window remains hidden"));
                }
            });
        }
    } else {
        window.show();
        logLine(QStringLiteral("window shown"));
    }
    logLine(QStringLiteral("entering event loop"));
    return app.exec();
}
