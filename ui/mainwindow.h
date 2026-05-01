#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QIcon>
#include <QMainWindow>
#include <QMap>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QVector>
class Database;
class HourlyChartWidget;
class QListWidget;
class QLabel;
class QCheckBox;
class QComboBox;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Database *database, QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override; // 拦截关闭事件
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupTrayIcon();                          // 初始化托盘

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
    QIcon iconForApp(const QString &appName) const;
    QString formatDuration(int seconds) const;

    Database *m_database = nullptr;
    QWidget *m_leftSidebar = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QSplitter *m_usageSplitter = nullptr;
    QListWidget *m_appStatsList = nullptr;
    QPushButton *m_dailyButton = nullptr;
    QPushButton *m_weeklyButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_sidebarToggleButton = nullptr;
    QCheckBox *m_autoStartSwitch = nullptr;
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

    QSystemTrayIcon *m_trayIcon = nullptr;  // 托盘图标
    QMenu *m_trayMenu = nullptr;            // 托盘右键菜单
};
#endif // MAINWINDOW_H
