#include <QApplication>
#include <QSettings>
#include "core/database.h"
#include "core/tracker.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // 关键：关闭窗口不退出程序

    Database database;
    database.init();
    Tracker tracker(&database);
    tracker.start();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &tracker, &Tracker::stop);

    MainWindow window(&database);
    const bool launchedByAutoStart = app.arguments().contains(QStringLiteral("--autostart"));
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
