#include <QApplication>
#include <QSettings>
#include "core/database.h"
#include "core/startupmanager.h"
#include "core/tracker.h"
#include "ui/mainwindow.h"
#include "ui/theme/thememanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("ScreenTime"));
    app.setApplicationName(QStringLiteral("ScreenTime"));
    app.setQuitOnLastWindowClosed(false); // 关键：关闭窗口不退出程序

    // 主题与界面完全解耦：全局样式 / 调色板由 ThemeManager 统一提供，
    // 主题切换通过 themeChanged 信号通知各控件自行刷新。
    ThemeManager::instance().applyTo(&app);

    Database database;
    database.init();
    Tracker tracker(&database);
    tracker.start();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &tracker, &Tracker::stop);

    MainWindow window(&database);
    // 兼容两种自启动方式：传统 exe 的 --autostart 参数，以及 MSIX startupTask
    // 激活（后者没有命令行参数，需要通过 AppInstance 激活参数判断）。
    const bool launchedByAutoStart = StartupManager::launchedByAutoStart();
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const QString launchMode =
        settings.value(QStringLiteral("startup/launch_mode"), QStringLiteral("tray")).toString();

    if (launchedByAutoStart && launchMode == QStringLiteral("tray")) {
        window.hide();
    } else {
        window.show();
    }
    return app.exec();
}
