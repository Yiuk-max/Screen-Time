#ifndef STARTUPMANAGER_H
#define STARTUPMANAGER_H

#include <QString>

class StartupManager {
public:
  static bool isAutoStartEnabled();
  static bool setAutoStartEnabled(bool enabled);
  static bool launchedByAutoStart();

  // WinRT helper entry point used by the packaged (MSIX) build. It runs in a
  // short-lived child process so a broken StartupTask activation cannot take
  // down the main application.
  static int runWinRtHelper(const QString &operation);

private:
  StartupManager() = delete;
};

#endif // STARTUPMANAGER_H
