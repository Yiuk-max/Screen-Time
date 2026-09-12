#ifndef STARTUPMANAGER_H
#define STARTUPMANAGER_H

class StartupManager {
public:
  static bool isAutoStartEnabled();
  static bool setAutoStartEnabled(bool enabled);
  static bool launchedByAutoStart();

private:
  StartupManager() = delete;
};

#endif // STARTUPMANAGER_H
