#include <QApplication>
#include <QSettings>

#include "core/database.h"
#include "core/startupmanager.h"
#include "core/tracker.h"
#include "ui/i18n/translationmanager.h"
#include "ui/mainwindow.h"
#include "ui/theme/thememanager.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("ScreenTime"));
  app.setApplicationName(QStringLiteral("ScreenTime"));
  app.setQuitOnLastWindowClosed(false);

  ThemeManager::instance().applyTo(&app);
  TranslationManager::instance();

  Database database;
  database.init();

  Tracker tracker(&database);
  tracker.start();
  QObject::connect(&app, &QCoreApplication::aboutToQuit, &tracker,
                   &Tracker::stop);

  MainWindow window(&database);
  QSettings settings(QStringLiteral("ScreenTime"),
                     QStringLiteral("ScreenTime"));
  const bool startInTray = StartupManager::launchedByAutoStart() &&
                           settings.value(QStringLiteral("startup/launch_mode"),
                                          QStringLiteral("tray"))
                                   .toString() == QStringLiteral("tray");

  if (startInTray) {
    window.hide();
  } else {
    window.show();
  }

  return app.exec();
}
