#include "mainwindow.h"
#include "core/database.h"
#include "hourlychartwidget.h"
#include <QApplication>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QButtonGroup>

#include <QCoreApplication>
#include <QCheckBox>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileIconProvider>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QSettings>
#include <QSvgRenderer>
#include <QStandardPaths>
#include <QStyle>
#include <algorithm>
#include <numeric>
#include <QPainter>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(Database *database, QWidget *parent)
    : QMainWindow(parent)
    , m_database(database)
{
    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    m_leftSidebar = new QWidget(central);
    m_leftSidebar->setFixedWidth(190);
    auto *sidebarLayout = new QVBoxLayout(m_leftSidebar);
    sidebarLayout->setContentsMargins(4, 4, 4, 4);
    sidebarLayout->setSpacing(8);

    m_sidebarToggleButton = new QPushButton(QStringLiteral("☰"), m_leftSidebar);
    m_sidebarToggleButton->setFixedSize(40, 40);
    m_sidebarToggleButton->setCursor(Qt::PointingHandCursor);
    m_sidebarToggleButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: rgb(210,210,215);"
        "  background-color: rgb(42,42,46);"
        "  border: 1px solid rgb(58,58,64);"
        "  border-radius: 20px;"
        "  font-size: 18px;"
        "}"
        "QPushButton:hover { background-color: rgb(52,52,58); }"));
    sidebarLayout->addWidget(m_sidebarToggleButton, 0, Qt::AlignLeft | Qt::AlignTop);

    sidebarLayout->addStretch();

    m_settingsButton = new QPushButton(m_leftSidebar);
    m_settingsButton->setFixedSize(176, 48);
    m_settingsButton->setCheckable(true);
    m_settingsButton->setText(QStringLiteral("  设置和帮助"));
    m_settingsButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: rgb(220,220,225);"
        "  background-color: rgb(34,34,38);"
        "  border: 1px solid rgb(48,48,52);"
        "  border-radius: 8px;"
        "  text-align: left;"
        "  padding-left: 10px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:checked {"
        "  background-color: rgb(44,44,48);"
        "  border-color: rgb(76,76,84);"
        "}"
        "QPushButton:hover {"
        "  background-color: rgb(42,42,46);"
        "}"));

    const QByteArray gearSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="#D4D4D8" d="M19.14 12.94c.04-.31.06-.63.06-.94s-.02-.63-.06-.94l2.03-1.58a.5.5 0 0 0 .12-.64l-1.92-3.32a.5.5 0 0 0-.6-.22l-2.39.96a7.1 7.1 0 0 0-1.63-.94L14.4 2.8a.5.5 0 0 0-.49-.4h-3.84a.5.5 0 0 0-.49.4l-.36 2.52c-.58.23-1.12.54-1.63.94l-2.39-.96a.5.5 0 0 0-.6.22L2.68 8.84a.5.5 0 0 0 .12.64l2.03 1.58c-.04.31-.06.63-.06.94s.02.63.06.94L2.8 14.52a.5.5 0 0 0-.12.64l1.92 3.32c.13.23.4.32.64.22l2.39-.96c.5.4 1.05.72 1.63.94l.36 2.52c.04.24.25.4.49.4h3.84c.24 0 .45-.16.49-.4l.36-2.52c.58-.23 1.12-.54 1.63-.94l2.39.96c.24.1.51.01.64-.22l1.92-3.32a.5.5 0 0 0-.12-.64l-2.03-1.58ZM12 15.5A3.5 3.5 0 1 1 12 8.5a3.5 3.5 0 0 1 0 7Z"/>
</svg>
)SVG";
    QSvgRenderer svgRenderer(gearSvg);
    QPixmap gearPixmap(18, 18);
    gearPixmap.fill(Qt::transparent);
    {
        QPainter painter(&gearPixmap);
        svgRenderer.render(&painter);
    }
    m_settingsButton->setIcon(QIcon(gearPixmap));
    m_settingsButton->setIconSize(QSize(18, 18));
    sidebarLayout->addWidget(m_settingsButton, 0, Qt::AlignLeft | Qt::AlignBottom);

    m_contentStack = new QStackedWidget(central);
    m_contentStack->addWidget(createUsagePage());
    m_contentStack->addWidget(createSettingsPage());

    layout->addWidget(m_leftSidebar);
    layout->addWidget(m_contentStack, 1);

    setCentralWidget(central);
    setWindowTitle(QStringLiteral("Screen Time"));
    resize(900, 600);

    connect(m_settingsButton, &QPushButton::toggled, this, [this](bool checked) {
        m_contentStack->setCurrentIndex(checked ? 1 : 0);
    });
    connect(m_sidebarToggleButton, &QPushButton::clicked, this, [this]() {
        applySidebarMode(!m_sidebarExpanded);
    });

    applySidebarMode(true);
    m_contentStack->setCurrentIndex(0);
    setupTrayIcon();
}

QWidget *MainWindow::createUsagePage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto *switchRow = new QHBoxLayout();
    m_dailyButton = new QPushButton(QStringLiteral("每日"), page);
    m_weeklyButton = new QPushButton(QStringLiteral("近7天"), page);
    m_dailyButton->setCheckable(true);
    m_weeklyButton->setCheckable(true);

    auto *periodGroup = new QButtonGroup(page);
    periodGroup->setExclusive(true);
    periodGroup->addButton(m_dailyButton);
    periodGroup->addButton(m_weeklyButton);
    m_dailyButton->setChecked(true);

    switchRow->addWidget(m_dailyButton);
    switchRow->addWidget(m_weeklyButton);
    switchRow->addStretch();

    auto *chartPanel = new QFrame(page);
    chartPanel->setFrameShape(QFrame::StyledPanel);
    chartPanel->setStyleSheet(QStringLiteral("QFrame { background-color: rgb(36,36,40); border-radius: 12px; }"));
    auto *chartLayout = new QVBoxLayout(chartPanel);
    m_primaryStatLabel = new QLabel(chartPanel);
    m_secondaryStatLabel = new QLabel(chartPanel);
    m_hourlyChartWidget = new HourlyChartWidget(chartPanel);
    m_hourlyChartWidget->setMinimumHeight(320);
    chartLayout->addWidget(m_primaryStatLabel);
    chartLayout->addWidget(m_secondaryStatLabel);
    chartLayout->addWidget(m_hourlyChartWidget);

    auto *statsTitle = new QLabel(QStringLiteral("应用统计"), page);
    m_appStatsList = new QListWidget(page);
    m_refreshButton = new QPushButton(QStringLiteral("刷新"), page);
    m_refreshButton->setFixedSize(90, 34);

    layout->addLayout(switchRow);
    layout->addWidget(chartPanel);
    layout->addWidget(statsTitle);
    layout->addWidget(m_appStatsList, 1);
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();
    bottomRow->addWidget(m_refreshButton);
    layout->addLayout(bottomRow);

    connect(m_dailyButton, &QPushButton::clicked, this, &MainWindow::fillAppStatsForDaily);
    connect(m_weeklyButton, &QPushButton::clicked, this, &MainWindow::fillAppStatsForWeekly);
    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        refreshUsageData();
    });

    m_refreshTimer.setInterval(5000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshUsageData);
    m_refreshTimer.start();
    refreshUsageData();
    fillAppStatsForDaily();
    return page;
}

QWidget *MainWindow::createSettingsPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    auto *title = new QLabel(QStringLiteral("设置"), page);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));

    auto *autoStartRow = new QFrame(page);
    autoStartRow->setStyleSheet(QStringLiteral(
        "QFrame { background-color: rgb(36,36,40); border-radius: 10px; }"
        "QLabel { color: rgb(235,235,240); font-size: 14px; }"));
    auto *rowLayout = new QHBoxLayout(autoStartRow);
    rowLayout->setContentsMargins(14, 12, 14, 12);
    rowLayout->setSpacing(12);

    auto *label = new QLabel(QStringLiteral("开机自启动"), autoStartRow);
    m_autoStartSwitch = new QCheckBox(autoStartRow);
    m_autoStartSwitch->setCursor(Qt::PointingHandCursor);
    m_autoStartSwitch->setStyleSheet(QStringLiteral(
        "QCheckBox::indicator {"
        "  width: 44px; height: 24px; border-radius: 12px;"
        "  background-color: rgb(95,95,102);"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: rgb(70,190,90);"
        "}"
        "QCheckBox::indicator::unchecked {"
        "  image: none;"
        "}"
        "QCheckBox::indicator::checked {"
        "  image: none;"
        "}"
        "QCheckBox::indicator {"
        "  border: 1px solid rgb(76,76,82);"
        "}"
        "QCheckBox::indicator:checked {"
        "  border: 1px solid rgb(70,190,90);"
        "}"
    ));

    // Add a moving thumb effect with a child label.
    auto *thumb = new QLabel(m_autoStartSwitch);
    thumb->setFixedSize(18, 18);
    thumb->setStyleSheet(QStringLiteral("background-color: white; border-radius: 9px;"));
    thumb->move(3, 3);
    connect(m_autoStartSwitch, &QCheckBox::toggled, thumb, [thumb](bool checked) {
        thumb->move(checked ? 23 : 3, 3);
    });

    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(m_autoStartSwitch);

    const bool enabled = isAutoStartEnabled();
    m_autoStartSwitch->setChecked(enabled);
    thumb->move(enabled ? 23 : 3, 3);
    connect(m_autoStartSwitch, &QCheckBox::toggled, this, [this](bool checked) {
        if (!setAutoStartEnabled(checked) && m_autoStartSwitch) {
            m_autoStartSwitch->blockSignals(true);
            m_autoStartSwitch->setChecked(!checked);
            m_autoStartSwitch->blockSignals(false);
        }
    });

    layout->addWidget(title);
    layout->addWidget(autoStartRow);
    layout->addStretch();
    return page;
}

void MainWindow::fillAppStatsForDaily()
{
    refreshAppStatsList(m_dailyAppStats);
    updateDailyChartAndSummary();
}

void MainWindow::fillAppStatsForWeekly()
{
    refreshAppStatsList(m_weeklyAppStats);
    updateWeeklySummary();
}

void MainWindow::refreshAppStatsList(const QVector<QPair<QString, int>> &items)
{
    if (!m_appStatsList) {
        return;
    }

    QVector<QPair<QString, int>> sortedItems = items;
    std::sort(sortedItems.begin(), sortedItems.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    m_appStatsList->clear();
    for (const auto &item : sortedItems) {
        auto *row = new QListWidgetItem(iconForApp(item.first),
                                        QStringLiteral("%1  -  %2").arg(item.first, formatDuration(item.second)));
        m_appStatsList->addItem(row);
    }
}

void MainWindow::refreshUsageData()
{
    if (!m_database) {
        return;
    }

    const QList<UsageRecord> todayRecords = m_database->queryToday();
    const QList<UsageRecord> weeklyRecords = m_database->queryWeekly();

    QHash<QString, int> dailyAppSeconds;
    QHash<QString, int> weeklyAppSeconds;
    m_appPathByName.clear();
    m_dailyMinutesByHour = QVector<int>(24, 0);
    m_dailyTopAppsByHour.clear();
    m_weeklyMinutesByDay = QVector<int>(7, 0);
    QVector<int> dailySecondsByHour(24, 0);
    QMap<int, QHash<QString, int>> dailyAppSecondsByHour;

    const QDate today = QDate::currentDate();
    const QDate weekStart = today.addDays(-6);

    for (const UsageRecord &record : todayRecords) {
        dailyAppSeconds[record.appName] += record.durationSeconds;
        if (!record.appPath.isEmpty()) {
            m_appPathByName[record.appName] = record.appPath;
        }
        const QDateTime tracked = QDateTime::fromString(record.trackedAt, "yyyy-MM-dd HH:mm:ss");
        if (!tracked.isValid()) {
            continue;
        }
        const int hour = tracked.time().hour();
        if (hour >= 0 && hour < 24) {
            dailySecondsByHour[hour] += record.durationSeconds;
            dailyAppSecondsByHour[hour][record.appName] += record.durationSeconds;
        }
    }

    for (const UsageRecord &record : weeklyRecords) {
        weeklyAppSeconds[record.appName] += record.durationSeconds;
        if (!record.appPath.isEmpty() && !m_appPathByName.contains(record.appName)) {
            m_appPathByName[record.appName] = record.appPath;
        }
        const QDate date = QDate::fromString(record.date, "yyyy-MM-dd");
        if (!date.isValid()) {
            continue;
        }
        const int index = weekStart.daysTo(date);
        if (index >= 0 && index < 7) {
            m_weeklyMinutesByDay[index] += (record.durationSeconds + 59) / 60;
        }
    }

    for (int hour = 0; hour < dailySecondsByHour.size(); ++hour) {
        m_dailyMinutesByHour[hour] = (dailySecondsByHour[hour] + 59) / 60;

        QVector<QPair<QString, int>> list;
        const auto appSeconds = dailyAppSecondsByHour.value(hour);
        for (auto it = appSeconds.cbegin(); it != appSeconds.cend(); ++it) {
            list.append({it.key(), (it.value() + 59) / 60});
        }
        std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
        if (list.size() > 3) {
            list.resize(3);
        }
        if (!list.isEmpty()) {
            m_dailyTopAppsByHour[hour] = list;
        }
    }

    m_dailyAppStats.clear();
    for (auto it = dailyAppSeconds.cbegin(); it != dailyAppSeconds.cend(); ++it) {
        m_dailyAppStats.append({it.key(), it.value()});
    }

    m_weeklyAppStats.clear();
    for (auto it = weeklyAppSeconds.cbegin(); it != weeklyAppSeconds.cend(); ++it) {
        m_weeklyAppStats.append({it.key(), it.value()});
    }

    if (m_dailyButton && m_dailyButton->isChecked()) {
        fillAppStatsForDaily();
    } else {
        fillAppStatsForWeekly();
    }
}

void MainWindow::updateDailyChartAndSummary()
{
    if (!m_hourlyChartWidget || !m_primaryStatLabel || !m_secondaryStatLabel) {
        return;
    }

    const int totalMinutes = std::accumulate(m_dailyMinutesByHour.cbegin(), m_dailyMinutesByHour.cend(), 0);
    m_primaryStatLabel->setText(QStringLiteral("今日总时长 %1").arg(formatDuration(totalMinutes * 60)));
    m_secondaryStatLabel->setText(QStringLiteral("使用时长（截至当前）"));
    QStringList hourLabels;
    for (int i = 0; i < 24; ++i) {
        hourLabels.append(QStringLiteral("%1时").arg(i));
    }
    m_hourlyChartWidget->setChartData(m_dailyMinutesByHour, hourLabels, m_dailyTopAppsByHour, true, 60, 30);
}

void MainWindow::updateWeeklySummary()
{
    if (!m_hourlyChartWidget || !m_primaryStatLabel || !m_secondaryStatLabel) {
        return;
    }

    const int totalMinutes = std::accumulate(m_weeklyMinutesByDay.cbegin(), m_weeklyMinutesByDay.cend(), 0);
    const int avgMinutes = m_weeklyMinutesByDay.isEmpty() ? 0 : totalMinutes / m_weeklyMinutesByDay.size();
    m_primaryStatLabel->setText(QStringLiteral("近7天日均 %1").arg(formatDuration(avgMinutes * 60)));
    m_secondaryStatLabel->setText(QStringLiteral("近7天总时长 %1").arg(formatDuration(totalMinutes * 60)));

    QStringList dayLabels;
    const QDate start = QDate::currentDate().addDays(-6);
    for (int i = 0; i < 7; ++i) {
        dayLabels.append(start.addDays(i).toString("MM-dd"));
    }

    int maxWeeklyMinutes = 30;
    for (int value : m_weeklyMinutesByDay) {
        maxWeeklyMinutes = qMax(maxWeeklyMinutes, value);
    }

    int dynamicMax = 120;
    int tick = 30;
    if (maxWeeklyMinutes <= 120) {
        dynamicMax = ((maxWeeklyMinutes + 29) / 30) * 30;
        tick = 30;
    } else if (maxWeeklyMinutes <= 360) {
        dynamicMax = ((maxWeeklyMinutes + 59) / 60) * 60;
        tick = 60;
    } else if (maxWeeklyMinutes <= 720) {
        dynamicMax = ((maxWeeklyMinutes + 119) / 120) * 120;
        tick = 120;
    } else {
        dynamicMax = ((maxWeeklyMinutes + 179) / 180) * 180;
        tick = 180;
    }

    m_hourlyChartWidget->setChartData(m_weeklyMinutesByDay, dayLabels, {}, false, dynamicMax, tick);
}

QString MainWindow::formatDuration(int seconds) const
{
    const int hours = seconds / 3600;
    const int minutes = (seconds % 3600) / 60;

    if (hours > 0) {
        return QStringLiteral("%1小时%2分钟").arg(hours).arg(minutes);
    }
    return QStringLiteral("%1分钟").arg(minutes);
}

QIcon MainWindow::iconForApp(const QString &appName) const
{
    QFileIconProvider provider;
    const QString directPath = m_appPathByName.value(appName);
    if (!directPath.isEmpty() && QFile::exists(directPath)) {
        return provider.icon(QFileInfo(directPath));
    }
    const QString executablePath = QStandardPaths::findExecutable(appName);
    if (!executablePath.isEmpty() && QFile::exists(executablePath)) {
        return provider.icon(QFileInfo(executablePath));
    }
    return style()->standardIcon(QStyle::SP_FileIcon);
}

bool MainWindow::isAutoStartEnabled() const
{
#ifdef Q_OS_WIN
    QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
    return runKey.contains(QStringLiteral("ScreenTime"));
#else
    return false;
#endif
}

bool MainWindow::setAutoStartEnabled(bool enabled) const
{
#ifdef Q_OS_WIN
    QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
    const QString key = QStringLiteral("ScreenTime");
    if (enabled) {
        const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        const QString qtBinPath =
            QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath));
        const QString command = QStringLiteral(
                                    "cmd /C \"set \"PATH=%1;%%PATH%%\" && \"%2\"\"")
                                    .arg(qtBinPath, appPath);
        runKey.setValue(key, command);
    } else {
        runKey.remove(key);
    }
    runKey.sync();
    return runKey.status() == QSettings::NoError;
#else
    Q_UNUSED(enabled);
    return false;
#endif
}
void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);

    // 用齿轮 SVG 作为托盘图标（和设置按钮同款）
    const QByteArray iconSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <rect width="24" height="24" rx="4" fill="#1a1a2e"/>
  <circle cx="12" cy="12" r="3" fill="#4a9eff"/>
  <path fill="#4a9eff" d="M12 2a1 1 0 0 1 1 1v1.07A8 8 0 0 1 19.93 11H21a1 1 0 0 1 0 2h-1.07A8 8 0 0 1 13 19.93V21a1 1 0 0 1-2 0v-1.07A8 8 0 0 1 4.07 13H3a1 1 0 0 1 0-2h1.07A8 8 0 0 1 11 4.07V3a1 1 0 0 1 1-1zm0 4a6 6 0 1 0 0 12A6 6 0 0 0 12 6z"/>
</svg>
)SVG";
    QSvgRenderer renderer(iconSvg);
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    m_trayIcon->setIcon(QIcon(pixmap));
    setWindowIcon(QIcon(pixmap));

    m_trayIcon->setToolTip(QStringLiteral("Screen Time"));

    // 右键菜单
    m_trayMenu = new QMenu(this);
    m_trayMenu->setStyleSheet(QStringLiteral(
        "QMenu {"
        "  background-color: rgb(36,36,40);"
        "  color: rgb(220,220,225);"
        "  border: 1px solid rgb(58,58,64);"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
        "QMenu::item { padding: 8px 20px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: rgb(52,52,58); }"));

    QAction *showAction = m_trayMenu->addAction(QStringLiteral("显示主窗口"));
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(QStringLiteral("退出"));

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    // 单击托盘图标显示/隐藏窗口
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    if (isVisible()) {
                        hide();
                    } else {
                        show();
                        raise();
                        activateWindow();
                    }
                }
            });

    connect(showAction, &QAction::triggered, this, [this]() {
        show();
        raise();
        activateWindow();
    });

    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 点击关闭按钮时最小化到托盘，而不是退出
    event->ignore();
    hide();
    m_trayIcon->showMessage(
        QStringLiteral("Screen Time"),
        QStringLiteral("程序已最小化到托盘，仍在后台记录使用时间"),
        QSystemTrayIcon::Information,
        2000);
}

void MainWindow::applySidebarMode(bool expanded)
{
    m_sidebarExpanded = expanded;
    if (!m_leftSidebar || !m_settingsButton || !m_sidebarToggleButton) {
        return;
    }

    if (expanded) {
        m_leftSidebar->setFixedWidth(190);
        m_settingsButton->setFixedSize(176, 48);
        m_settingsButton->setText(QStringLiteral("  设置和帮助"));
        m_settingsButton->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: rgb(220,220,225);"
            "  background-color: rgb(34,34,38);"
            "  border: 1px solid rgb(48,48,52);"
            "  border-radius: 8px;"
            "  text-align: left;"
            "  padding-left: 10px;"
            "  font-size: 14px;"
            "}"
            "QPushButton:checked {"
            "  background-color: rgb(44,44,48);"
            "  border-color: rgb(76,76,84);"
            "}"
            "QPushButton:hover {"
            "  background-color: rgb(42,42,46);"
            "}"));
        m_sidebarToggleButton->setText(QStringLiteral("☰"));
    } else {
        m_leftSidebar->setFixedWidth(68);
        m_settingsButton->setFixedSize(44, 44);
        m_settingsButton->setText(QString());
        m_settingsButton->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: rgb(220,220,225);"
            "  background-color: rgb(34,34,38);"
            "  border: 1px solid rgb(48,48,52);"
            "  border-radius: 8px;"
            "  text-align: center;"
            "}"
            "QPushButton:checked {"
            "  background-color: rgb(44,44,48);"
            "  border-color: rgb(76,76,84);"
            "}"
            "QPushButton:hover {"
            "  background-color: rgb(42,42,46);"
            "}"));
        m_sidebarToggleButton->setText(QStringLiteral("≡"));
    }    
}
