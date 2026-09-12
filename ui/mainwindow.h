
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QIcon>
#include <QMainWindow>
#include <QPixmap>
#include <QMap>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QVector>
#include "fluenttoggleswitch.h"
#include "theme/theme.h"
class Database;
class HourlyChartWidget;
class QListWidget;
class QLabel;
class QComboBox;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSlider;
class QFrame;
class QToolButton;
class QAction;
class AIReportPage;
class QShowEvent;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Database *database, QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override; // 拦截关闭事件
    void changeEvent(QEvent *event) override;      // 拦截最小化事件
    void showEvent(QShowEvent *event) override;   // 显示后再同步一次 DWM 标题栏
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupTrayIcon();                          // 初始化托盘
    void ensureTrayIconVisible();                  // 登录早期等待 Explorer 托盘就绪
    void hideToTray();                             // 隐藏到系统托盘并提示

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

private:
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

    QSystemTrayIcon *m_trayIcon = nullptr;  // 托盘图标
    QMenu *m_trayMenu = nullptr;            // 托盘右键菜单
    QTimer m_trayRetryTimer;                // 开机登录时等待系统托盘完成初始化
    int m_trayRetryAttempts = 0;
    QAction *m_showAction = nullptr;        // 显示主窗口
    QAction *m_quitAction = nullptr;        // 退出
    QToolButton *m_shareButton = nullptr;   // 项目链接按钮

    AIReportPage *m_aiReportPage = nullptr;
    QLabel *m_versionLabel = nullptr;

    // AI 周报配置（设置页面独立栏目）
    FluentToggleSwitch *m_aiReportEnabledSwitch = nullptr;
    QLineEdit *m_deepseekApiKeyEdit = nullptr;
};
#endif // MAINWINDOW_H
