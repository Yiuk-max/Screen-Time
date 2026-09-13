#include <QApplication>
#include <QSettings>

#include "core/database.h"
#include "core/startupmanager.h"
#include "core/tracker.h"
#include "ui/i18n/translationmanager.h"
#include "ui/mainwindow.h"
#include "ui/theme/thememanager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  // WinRT startup toggle helper: exits immediately without creating any Qt
  // objects, so a faulty StartupTask activation cannot crash the main app.
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
  app.setOrganizationName(QStringLiteral("ScreenTime"));
  app.setApplicationName(QStringLiteral("ScreenTime"));
  app.setQuitOnLastWindowClosed(false);

  // Capture the launch origin before heavy initialization; the system process
  // that starts the MSIX StartupTask may be short-lived.
  const bool launchedByAutoStart = StartupManager::launchedByAutoStart();

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
  const bool startInTray = launchedByAutoStart &&
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
