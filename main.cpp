#include <QApplication>
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
    window.show();
    return app.exec();
}
