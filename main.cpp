#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QSystemTrayIcon>

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
#  include <roapi.h>
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
    logLine(QStringLiteral("!! CRASH code=0x%1 address=0x%2")
                .arg(code, 8, 16, QLatin1Char('0'))
                .arg(address, 0, 16));
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 关键：必须在 Qt 初始化 OLE 之前，把主线程初始化为 WinRT 单线程套间
    // (ASTA)。否则 MSIX 环境下 Windows.ApplicationModel.*（StartupTask /
    // AppInstance）会因套间不匹配触发访问违规(0xC0000005)。
    RoInitialize(RO_INIT_SINGLETHREADED);
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

    // 兼容两种自启动方式：传统 exe 的 --autostart 参数，以及 MSIX startupTask
    // 激活（后者没有命令行参数，需要通过 AppInstance 激活参数判断）。
    const bool launchedByAutoStart = StartupManager::launchedByAutoStart();
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const QString launchMode =
        settings.value(QStringLiteral("startup/launch_mode"), QStringLiteral("tray")).toString();
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    logLine(QStringLiteral("launchedByAutoStart=%1 launchMode=%2 trayAvailable=%3")
                .arg(launchedByAutoStart).arg(launchMode).arg(trayAvailable));

    // 只有在“自启动 + 托盘模式 + 托盘可用”时才隐藏；否则一律显示窗口，
    // 避免因判断异常导致用户点开却看不到界面。
    if (launchedByAutoStart && launchMode == QStringLiteral("tray") && trayAvailable) {
        window.hide();
        logLine(QStringLiteral("window hidden to tray"));
    } else {
        window.show();
        logLine(QStringLiteral("window shown"));
    }
    logLine(QStringLiteral("entering event loop"));
    return app.exec();
}
