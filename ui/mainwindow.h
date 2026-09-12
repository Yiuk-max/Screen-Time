#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QIcon>
#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QVector>

#include "fluenttoggleswitch.h"
#include "theme/theme.h"

class QAction;
class AIReportPage;
class Database;
class HourlyChartWidget;
class QCloseEvent;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QPushButton;
class QScrollArea;
class QShowEvent;
class QSplitter;
class QStackedWidget;
class QSystemTrayIcon;
class QToolButton;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(Database *database, QWidget *parent = nullptr);
  ~MainWindow() override = default;

protected:
  void closeEvent(QCloseEvent *event) override;
  void changeEvent(QEvent *event) override;
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void setupTrayIcon();
  void hideToTray();

  QWidget *createUsagePage();
  QWidget *createSettingsPage();
  void fillAppStatsForDaily();
  void fillAppStatsForWeekly();
  void refreshAppStatsList(const QVector<QPair<QString, int>> &items);
  void refreshUsageData();
  void updateDailyChartAndSummary();
  void updateWeeklySummary();
  bool isAutoStartEnabled() const;
  bool setAutoStartEnabled(bool enabled) const;
  QString startupLaunchMode() const;
  void setStartupLaunchMode(const QString &mode) const;
  void applySidebarMode(bool expanded);
  void clampUsageSplitter();
  void onThemeChanged(const Theme &theme);
  void refreshIcons(const Theme &theme);
  void retranslateUi();
  void rebuildThemeCombo();
  void rebuildAccentCombo();
  void rebuildStartupModeCombo();
  void rebuildLanguageCombo();
  void syncAIReportSettings();
  QIcon iconForApp(const QString &appName) const;
  QString formatDuration(int seconds) const;

  Database *m_database = nullptr;
  QWidget *m_leftSidebar = nullptr;
  QStackedWidget *m_contentStack = nullptr;
  QSplitter *m_usageSplitter = nullptr;
  QListWidget *m_appStatsList = nullptr;
  QPushButton *m_dailyButton = nullptr;
  QPushButton *m_weeklyButton = nullptr;
  QPushButton *m_homeButton = nullptr;
  QPushButton *m_aiReportButton = nullptr;
  QPushButton *m_settingsButton = nullptr;
  QPushButton *m_sidebarToggleButton = nullptr;
  FluentToggleSwitch *m_autoStartSwitch = nullptr;
  QComboBox *m_startupModeCombo = nullptr;
  QLabel *m_primaryStatLabel = nullptr;
  HourlyChartWidget *m_hourlyChartWidget = nullptr;
  QTimer m_refreshTimer;
  QVector<QPair<QString, int>> m_dailyAppStats;
  QVector<QPair<QString, int>> m_weeklyAppStats;
  QVector<int> m_dailyMinutesByHour;
  QMap<int, QVector<QPair<QString, int>>> m_dailyTopAppsByHour;
  QVector<int> m_weeklyMinutesByDay;
  QMap<int, QVector<QPair<QString, int>>> m_weeklyTopAppsByDay;
  QMap<QString, QIcon> m_appIconByName;
  QMap<QString, QString> m_appPathByName;
  bool m_sidebarExpanded = true;
  Theme m_theme;

  QFrame *m_chartPanel = nullptr;
  QLabel *m_statsTitleLabel = nullptr;
  QScrollArea *m_settingsScrollArea = nullptr;
  QWidget *m_settingsContent = nullptr;
  QComboBox *m_themeCombo = nullptr;
  QComboBox *m_accentCombo = nullptr;
  QComboBox *m_languageCombo = nullptr;
  FluentToggleSwitch *m_notificationSwitch = nullptr;

  QSystemTrayIcon *m_trayIcon = nullptr;
  QMenu *m_trayMenu = nullptr;
  QAction *m_showAction = nullptr;
  QAction *m_quitAction = nullptr;
  QToolButton *m_shareButton = nullptr;

  AIReportPage *m_aiReportPage = nullptr;
  QLabel *m_versionLabel = nullptr;
  FluentToggleSwitch *m_aiReportEnabledSwitch = nullptr;
  QLineEdit *m_deepseekApiKeyEdit = nullptr;
};

#endif // MAINWINDOW_H
