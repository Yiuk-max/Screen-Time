#include "mainwindow.h"
#include "core/database.h"
#include "hourlychartwidget.h"
#include "aireportpage.h"
#include "apptheme.h"
#include <QApplication>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QButtonGroup>
#include <QMessageBox>

#include <QCoreApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
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
#include <QSettings>
#include <QSvgRenderer>
#include <QStandardPaths>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
#include <QToolTip>
#include <algorithm>
#include <numeric>
#include <QPainter>
#include <QSize>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QWidget>
#include <QScrollArea>

#ifndef SCREENTIME_VERSION
#define SCREENTIME_VERSION "0.1.0"
#endif

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
    m_sidebarToggleButton->setToolTip(QStringLiteral("展开菜单"));
    sidebarLayout->addWidget(m_sidebarToggleButton, 0, Qt::AlignLeft | Qt::AlignTop);

    // --- 主页按钮（最上方） ---
    m_homeButton = new QPushButton(m_leftSidebar);
    m_homeButton->setFixedSize(176, 48);
    m_homeButton->setCheckable(true);
    m_homeButton->setText(QStringLiteral("  主页"));
    m_homeButton->setStyleSheet(QStringLiteral(
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

    const QByteArray homeBtnSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="#D4D4D8" d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z"/>
</svg>
)SVG";
    QSvgRenderer homeRenderer(homeBtnSvg);
    QPixmap homePixmap(18, 18);
    homePixmap.fill(Qt::transparent);
    {
        QPainter painter(&homePixmap);
        homeRenderer.render(&painter);
    }
    m_homeButton->setIcon(QIcon(homePixmap));
    m_homeButton->setIconSize(QSize(18, 18));
    m_homeButton->setToolTip(QStringLiteral("返回主页"));
    sidebarLayout->addWidget(m_homeButton, 0, Qt::AlignLeft);

    connect(m_homeButton, &QPushButton::clicked, this, [this]() {
        m_homeButton->setChecked(true);
        if (m_aiReportButton) m_aiReportButton->setChecked(false);
        if (m_settingsButton) m_settingsButton->setChecked(false);
        m_contentStack->setCurrentIndex(0);
    });

    // --- 分析报告按钮（主页下方） ---
    m_aiReportButton = new QPushButton(m_leftSidebar);
    m_aiReportButton->setFixedSize(176, 48);
    m_aiReportButton->setCheckable(true);
    m_aiReportButton->setText(QStringLiteral("  分析报告"));
    m_aiReportButton->setStyleSheet(QStringLiteral(
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

    const QByteArray aiSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="#D4D4D8" d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8l-6-6zm-1 2.5L18.5 10H13V4.5zM8 12h8v1.5H8V12zm0 3h8v1.5H8V15zm0 3h5v1.5H8V18z"/>
</svg>
)SVG";
    QSvgRenderer aiRenderer(aiSvg);
    QPixmap aiPixmap(18, 18);
    aiPixmap.fill(Qt::transparent);
    {
        QPainter painter(&aiPixmap);
        aiRenderer.render(&painter);
    }
    m_aiReportButton->setIcon(QIcon(aiPixmap));
    m_aiReportButton->setIconSize(QSize(18, 18));
    m_aiReportButton->setToolTip(QStringLiteral("查看使用分析报告"));
    sidebarLayout->addWidget(m_aiReportButton, 0, Qt::AlignLeft);

    connect(m_aiReportButton, &QPushButton::clicked, this, [this]() {
        m_aiReportButton->setChecked(true);
        if (m_homeButton) m_homeButton->setChecked(false);
        if (m_settingsButton) m_settingsButton->setChecked(false);
        m_contentStack->setCurrentIndex(2);
    });

    sidebarLayout->addStretch();

    // --- 设置按钮（侧边最下面） ---
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
    m_settingsButton->setToolTip(QStringLiteral("设置和帮助"));
    sidebarLayout->addWidget(m_settingsButton, 0, Qt::AlignLeft | Qt::AlignBottom);

    connect(m_settingsButton, &QPushButton::clicked, this, [this]() {
        m_settingsButton->setChecked(true);
        if (m_homeButton) m_homeButton->setChecked(false);
        if (m_aiReportButton) m_aiReportButton->setChecked(false);
        m_contentStack->setCurrentIndex(1);
    });

    m_contentStack = new QStackedWidget(central);
    m_contentStack->setMinimumSize(0, 0);
    m_contentStack->addWidget(createUsagePage());
    m_contentStack->addWidget(createSettingsPage());

    // 创建分析报告页面（须在 createSettingsPage 之后，以便同步已保存的配置）
    m_aiReportPage = new AIReportPage(m_database, central);
    m_contentStack->addWidget(m_aiReportPage);
    syncAIReportSettings();
    applyCurrentTheme();

    layout->addWidget(m_leftSidebar);
    layout->addWidget(m_contentStack, 1);

    setCentralWidget(central);
    setWindowTitle(QStringLiteral("Screen Time"));
    resize(900, 660);
    setMinimumSize(760, 420);

    // 当点击设置或AI周报页面的内容区时，不自动切回主页
    connect(m_contentStack, &QStackedWidget::currentChanged, this, [this](int index) {
        if (index == 0) {
            m_homeButton->setChecked(true);
            m_settingsButton->setChecked(false);
            m_aiReportButton->setChecked(false);
        }
    });
    connect(m_sidebarToggleButton, &QPushButton::clicked, this, [this]() {
        applySidebarMode(!m_sidebarExpanded);
    });

    applySidebarMode(false);
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
    m_chartPanel = chartPanel;
    chartPanel->setFrameShape(QFrame::StyledPanel);
    auto *chartLayout = new QVBoxLayout(chartPanel);
    chartLayout->setContentsMargins(10, 10, 10, 10);
    chartLayout->setSpacing(6);
    m_primaryStatLabel = new QLabel(chartPanel);
    m_hourlyChartWidget = new HourlyChartWidget(chartPanel);
    m_hourlyChartWidget->setMinimumHeight(0);
    m_hourlyChartWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartLayout->addWidget(m_primaryStatLabel);
    chartLayout->addWidget(m_hourlyChartWidget, 1);

    auto *statsPanel = new QFrame(page);
    statsPanel->setFrameShape(QFrame::NoFrame);
    auto *statsLayout = new QVBoxLayout(statsPanel);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(8);
    auto *statsTitle = new QLabel(QStringLiteral("应用统计"), statsPanel);
    m_statsTitleLabel = statsTitle;
    m_appStatsList = new QListWidget(statsPanel);
    statsLayout->addWidget(statsTitle);
    statsLayout->addWidget(m_appStatsList, 1);

    m_usageSplitter = new QSplitter(Qt::Vertical, page);
    m_usageSplitter->setChildrenCollapsible(false);
    m_usageSplitter->setHandleWidth(8);
    m_usageSplitter->addWidget(chartPanel);
    m_usageSplitter->addWidget(statsPanel);
    m_usageSplitter->setStretchFactor(0, 3);
    m_usageSplitter->setStretchFactor(1, 2);
    m_usageSplitter->setSizes({280, 180});

    chartPanel->setMinimumHeight(0);
    statsPanel->setMinimumHeight(0);
    m_appStatsList->setMinimumHeight(0);

    if (QSplitterHandle *handle = m_usageSplitter->handle(1)) {
        handle->installEventFilter(this);
        handle->setCursor(Qt::SizeVerCursor);
        handle->setToolTip(QStringLiteral("拖动调整柱状图和应用统计区域大小"));
    }
    connect(m_usageSplitter, &QSplitter::splitterMoved, this, [this](int, int) {
        clampUsageSplitter();
    });

    layout->addLayout(switchRow);
    layout->addWidget(m_usageSplitter, 1);

    connect(m_dailyButton, &QPushButton::clicked, this, &MainWindow::fillAppStatsForDaily);
    connect(m_weeklyButton, &QPushButton::clicked, this, &MainWindow::fillAppStatsForWeekly);

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
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto *scrollArea = new QScrollArea(page);
    m_settingsScrollArea = scrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scrollArea);
    m_settingsContent = content;
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    scrollArea->setMinimumSize(0, 0);
    content->setMinimumSize(0, 0);

    auto *title = new QLabel(QStringLiteral("设置"), content);
    title->setObjectName(QStringLiteral("settingsPageTitle"));
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600;"));

    auto *themeRow = new QFrame(content);
    themeRow->setObjectName(QStringLiteral("settingsCard"));
    auto *themeLayout = new QHBoxLayout(themeRow);
    themeLayout->setContentsMargins(14, 12, 14, 12);
    themeLayout->setSpacing(12);
    auto *themeLabel = new QLabel(QStringLiteral("界面主题"), themeRow);
    m_themeCombo = new QComboBox(themeRow);
    m_themeCombo->addItem(QStringLiteral("深色主题"), QStringLiteral("dark"));
    m_themeCombo->addItem(QStringLiteral("浅色主题"), QStringLiteral("light"));
    const AppThemeKind savedTheme = loadSavedThemeKind();
    m_themeCombo->setCurrentIndex(savedTheme == AppThemeKind::Light ? 1 : 0);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (!m_themeCombo) {
            return;
        }
        const QString value = m_themeCombo->currentData().toString();
        applyTheme(value == QStringLiteral("light") ? AppThemeKind::Light : AppThemeKind::Dark);
    });
    themeLayout->addWidget(themeLabel);
    themeLayout->addStretch();
    themeLayout->addWidget(m_themeCombo);
    layout->addWidget(title);
    layout->addWidget(themeRow);

    auto *autoStartRow = new QFrame(content);
    autoStartRow->setObjectName(QStringLiteral("settingsCard"));
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

    layout->addWidget(autoStartRow);

    auto *startupModeRow = new QFrame(content);
    startupModeRow->setObjectName(QStringLiteral("settingsCard"));
    auto *modeLayout = new QHBoxLayout(startupModeRow);
    modeLayout->setContentsMargins(14, 12, 14, 12);
    modeLayout->setSpacing(12);

    auto *modeLabel = new QLabel(QStringLiteral("开机启动方式"), startupModeRow);
    m_startupModeCombo = new QComboBox(startupModeRow);
    m_startupModeCombo->addItem(QStringLiteral("系统托盘启动"), QStringLiteral("tray"));
    m_startupModeCombo->addItem(QStringLiteral("弹出主界面"), QStringLiteral("window"));

    const QString mode = startupLaunchMode();
    const int modeIndex = qMax(0, m_startupModeCombo->findData(mode));
    m_startupModeCombo->setCurrentIndex(modeIndex);
    connect(m_startupModeCombo, &QComboBox::currentIndexChanged, this, [this]() {
        if (!m_startupModeCombo) {
            return;
        }
        setStartupLaunchMode(m_startupModeCombo->currentData().toString());
    });

    modeLayout->addWidget(modeLabel);
    modeLayout->addStretch();
    modeLayout->addWidget(m_startupModeCombo);

    layout->addWidget(startupModeRow);

    auto *githubRow = new QFrame(content);
    githubRow->setObjectName(QStringLiteral("settingsCard"));
    githubRow->setProperty("settingsCardVariant", QStringLiteral("link"));
    auto *githubLayout = new QHBoxLayout(githubRow);
    githubLayout->setContentsMargins(14, 12, 14, 12);
    githubLayout->setSpacing(10);

    auto *githubLabel = new QLabel(QStringLiteral("项目地址"), githubRow);
    auto *githubButton = new QPushButton(QStringLiteral("Yiuk-max/Screen-Time"), githubRow);
    githubButton->setCursor(Qt::PointingHandCursor);
    connect(githubButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Yiuk-max/Screen-Time")));
    });

    const QByteArray shareSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="#B8C8E8" d="M14 3a1 1 0 1 0 0 2h3.59l-7.3 7.3a1 1 0 1 0 1.42 1.4L19 6.41V10a1 1 0 1 0 2 0V4a1 1 0 0 0-1-1h-6Z"/>
  <path fill="#B8C8E8" d="M6 5a3 3 0 0 0-3 3v10a3 3 0 0 0 3 3h10a3 3 0 0 0 3-3v-5a1 1 0 1 0-2 0v5a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V8a1 1 0 0 1 1-1h5a1 1 0 1 0 0-2H6Z"/>
</svg>
)SVG";
    QSvgRenderer shareRenderer(shareSvg);
    QPixmap sharePix(16, 16);
    sharePix.fill(Qt::transparent);
    {
        QPainter painter(&sharePix);
        shareRenderer.render(&painter);
    }
    auto *shareButton = new QToolButton(githubRow);
    shareButton->setIcon(QIcon(sharePix));
    shareButton->setIconSize(QSize(18, 18));
    shareButton->setAutoRaise(true);
    shareButton->setToolTip(QStringLiteral("打开项目链接"));
    shareButton->setCursor(Qt::PointingHandCursor);
    connect(shareButton, &QToolButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Yiuk-max/Screen-Time")));
    });

    githubLayout->addWidget(githubLabel);
    githubLayout->addSpacing(8);
    githubLayout->addWidget(githubButton, 1);
    githubLayout->addWidget(shareButton, 0, Qt::AlignRight);

    layout->addWidget(githubRow);

    // ========== 分析报告配置区域（独立栏目） ==========
    auto *aiTitle = new QLabel(QStringLiteral("分析报告"), content);
    aiTitle->setObjectName(QStringLiteral("settingsSectionTitle"));
    aiTitle->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600; margin-top: 12px;"));

    // 启用开关
    auto *aiEnableRow = new QFrame(content);
    aiEnableRow->setObjectName(QStringLiteral("settingsCard"));
    auto *aiEnableLayout = new QHBoxLayout(aiEnableRow);
    aiEnableLayout->setContentsMargins(14, 12, 14, 12);
    aiEnableLayout->setSpacing(12);

    auto *aiEnableLabel = new QLabel(QStringLiteral("启用分析报告"), aiEnableRow);
    m_aiReportEnabledSwitch = new QCheckBox(aiEnableRow);
    m_aiReportEnabledSwitch->setCursor(Qt::PointingHandCursor);
    // 复制开关样式，并添加白色滑块效果
    const QString toggleSwitchStyle = QStringLiteral(
        "QCheckBox::indicator {"
        "  width: 44px; height: 24px; border-radius: 12px;"
        "  background-color: rgb(95,95,102);"
        "  border: 1px solid rgb(76,76,82);"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: rgb(70,190,90);"
        "  border: 1px solid rgb(70,190,90);"
        "}"
        "QCheckBox::indicator:unchecked {"
        "  image: none;"
        "}"
        "QCheckBox::indicator:checked {"
        "  image: none;"
        "}");
    m_aiReportEnabledSwitch->setStyleSheet(toggleSwitchStyle);

    // AI开关滑块
    auto *aiThumb = new QLabel(m_aiReportEnabledSwitch);
    aiThumb->setFixedSize(18, 18);
    aiThumb->setStyleSheet(QStringLiteral("background-color: white; border-radius: 9px;"));
    aiThumb->move(3, 3);
    connect(m_aiReportEnabledSwitch, &QCheckBox::toggled, aiThumb, [aiThumb](bool checked) {
        aiThumb->move(checked ? 23 : 3, 3);
    });

    aiEnableLayout->addWidget(aiEnableLabel);
    aiEnableLayout->addStretch();
    aiEnableLayout->addWidget(m_aiReportEnabledSwitch);

    layout->addWidget(aiTitle);
    layout->addWidget(aiEnableRow);

    // DeepSeek API Key 输入
    auto *apiKeyRow = new QFrame(content);
    apiKeyRow->setObjectName(QStringLiteral("settingsCard"));
    apiKeyRow->setProperty("settingsCardVariant", QStringLiteral("lineEdit"));
    auto *apiKeyLayout = new QHBoxLayout(apiKeyRow);
    apiKeyLayout->setContentsMargins(14, 12, 14, 12);
    apiKeyLayout->setSpacing(12);

    auto *apiKeyLabel = new QLabel(QStringLiteral("DeepSeek API Key"), apiKeyRow);
    m_deepseekApiKeyEdit = new QLineEdit(apiKeyRow);
    m_deepseekApiKeyEdit->setEchoMode(QLineEdit::Password);
    m_deepseekApiKeyEdit->setPlaceholderText(QStringLiteral("sk-..."));
    m_deepseekApiKeyEdit->setMinimumWidth(280);

    // 从 QSettings 加载已保存的 API Key
    QSettings apiSettings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const QString savedApiKey = apiSettings.value(QStringLiteral("ai/deepseek_api_key")).toString();
    if (!savedApiKey.isEmpty()) {
        m_deepseekApiKeyEdit->setText(savedApiKey);
    }

    // 文本变化时立即保存，不必等 editingFinished（焦点丢失）
    connect(m_deepseekApiKeyEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
        const QString key = text.trimmed();
        s.setValue(QStringLiteral("ai/deepseek_api_key"), key);
        s.sync();
        if (m_aiReportPage) {
            m_aiReportPage->setApiKey(key);
        }
    });

    // 同时也保留 editingFinished 以防万一
    connect(m_deepseekApiKeyEdit, &QLineEdit::editingFinished, this, [this]() {
        if (m_deepseekApiKeyEdit) {
            QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
            const QString key = m_deepseekApiKeyEdit->text().trimmed();
            s.setValue(QStringLiteral("ai/deepseek_api_key"), key);
            s.sync();
            if (m_aiReportPage) {
                m_aiReportPage->setApiKey(key);
            }
        }
    });

    apiKeyLayout->addWidget(apiKeyLabel);
    apiKeyLayout->addStretch();
    apiKeyLayout->addWidget(m_deepseekApiKeyEdit);

    layout->addWidget(apiKeyRow);

    // 连接启用开关
    connect(m_aiReportEnabledSwitch, &QCheckBox::toggled, this, [this](bool checked) {
        QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
        s.setValue(QStringLiteral("ai/enabled"), checked);
        syncAIReportSettings();
    });

    // 加载保存的启用状态（AI 页面创建后由 syncAIReportSettings 同步）
    QSettings aiToggledSettings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const bool aiEnabled = aiToggledSettings.value(QStringLiteral("ai/enabled"), false).toBool();
    m_aiReportEnabledSwitch->setChecked(aiEnabled);

    // ========== 关于 ==========
    auto *aboutTitle = new QLabel(QStringLiteral("帮助"), content);
    aboutTitle->setObjectName(QStringLiteral("settingsSectionTitle"));
    aboutTitle->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600; margin-top: 12px;"));

    auto *versionRow = new QFrame(content);
    versionRow->setObjectName(QStringLiteral("settingsCard"));
    auto *versionLayout = new QHBoxLayout(versionRow);
    versionLayout->setContentsMargins(14, 12, 14, 12);
    versionLayout->setSpacing(12);

    auto *versionLabel = new QLabel(QStringLiteral("当前版本"), versionRow);
    m_versionLabel = new QLabel(QStringLiteral(SCREENTIME_VERSION), versionRow);

    versionLayout->addWidget(versionLabel);
    versionLayout->addStretch();
    versionLayout->addWidget(m_versionLabel);

    layout->addWidget(aboutTitle);

    auto makeHelpRow = [&](const QString &labelText, const QString &buttonText, const auto &handler) {
        auto *row = new QFrame(content);
        row->setObjectName(QStringLiteral("settingsCard"));
        row->setProperty("settingsCardVariant", QStringLiteral("link"));
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 12, 14, 12);
        rowLayout->setSpacing(10);

        auto *label = new QLabel(labelText, row);
        auto *button = new QPushButton(buttonText, row);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, handler);

        rowLayout->addWidget(label);
        rowLayout->addStretch();
        rowLayout->addWidget(button);
        layout->addWidget(row);
    };

    makeHelpRow(QStringLiteral("关于"), QStringLiteral("Screen Time %1").arg(QStringLiteral(SCREENTIME_VERSION)), [this]() {
        QMessageBox::about(this,
                           QStringLiteral("关于 Screen Time"),
                           QStringLiteral("Screen Time\n版本：%1").arg(QStringLiteral(SCREENTIME_VERSION)));
    });
    makeHelpRow(QStringLiteral("隐私政策"), QStringLiteral("打开隐私政策"), []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://yiukblog.xyz")));
    });
    makeHelpRow(QStringLiteral("项目地址"), QStringLiteral("Yiuk-max/Screen-Time"), []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Yiuk-max/Screen-Time")));
    });

    layout->addWidget(versionRow);
    layout->addStretch();

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);
    return page;
}

void MainWindow::syncAIReportSettings()
{
    if (!m_aiReportPage) {
        return;
    }

    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const bool aiEnabled = m_aiReportEnabledSwitch
        ? m_aiReportEnabledSwitch->isChecked()
        : settings.value(QStringLiteral("ai/enabled"), false).toBool();
    const QString apiKey = settings.value(QStringLiteral("ai/deepseek_api_key")).toString();

    m_aiReportPage->setAIEnabled(aiEnabled);
    m_aiReportPage->setApiKey(apiKey);
}

void MainWindow::applyCurrentTheme()
{
    applyTheme(loadSavedThemeKind());
}

void MainWindow::applyTheme(AppThemeKind kind)
{
    m_theme = appThemeColors(kind);
    saveThemeKind(kind);

    if (m_themeCombo) {
        m_themeCombo->blockSignals(true);
        m_themeCombo->setCurrentIndex(kind == AppThemeKind::Light ? 1 : 0);
        m_themeCombo->blockSignals(false);
        m_themeCombo->setStyleSheet(comboBoxStyleSheet(m_theme));
    }

    setStyleSheet(windowStyleSheet(m_theme));
    if (QWidget *central = centralWidget()) {
        central->setStyleSheet(QStringLiteral("background-color: %1;").arg(m_theme.windowBackground));
    }
    if (m_leftSidebar) {
        m_leftSidebar->setStyleSheet(QStringLiteral("background-color: transparent;"));
    }
    if (m_sidebarToggleButton) {
        m_sidebarToggleButton->setStyleSheet(sidebarToggleStyleSheet(m_theme));
    }

    applySidebarMode(m_sidebarExpanded);

    if (m_dailyButton) {
        m_dailyButton->setStyleSheet(periodButtonStyleSheet(m_theme));
    }
    if (m_weeklyButton) {
        m_weeklyButton->setStyleSheet(periodButtonStyleSheet(m_theme));
    }
    if (m_chartPanel) {
        m_chartPanel->setStyleSheet(chartPanelStyleSheet(m_theme));
    }
    if (m_primaryStatLabel) {
        m_primaryStatLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 14px;").arg(m_theme.textPrimary));
    }
    if (m_hourlyChartWidget) {
        m_hourlyChartWidget->setChartTheme(m_theme.chart);
    }
    if (m_statsTitleLabel) {
        m_statsTitleLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 14px; font-weight: 600;")
                                             .arg(m_theme.textPrimary));
    }
    if (m_appStatsList) {
        m_appStatsList->setStyleSheet(listWidgetStyleSheet(m_theme));
    }
    if (m_usageSplitter) {
        m_usageSplitter->setStyleSheet(splitterStyleSheet(m_theme));
    }
    if (m_settingsScrollArea) {
        m_settingsScrollArea->setStyleSheet(scrollAreaStyleSheet(m_theme));
    }
    if (m_settingsContent) {
        for (QFrame *frame : m_settingsContent->findChildren<QFrame *>()) {
            if (frame->objectName() != QStringLiteral("settingsCard")) {
                continue;
            }
            const QString variant = frame->property("settingsCardVariant").toString();
            if (variant == QStringLiteral("link")) {
                frame->setStyleSheet(linkButtonStyleSheet(m_theme));
            } else if (variant == QStringLiteral("lineEdit")) {
                frame->setStyleSheet(lineEditStyleSheet(m_theme));
            } else if (variant == QStringLiteral("secondaryButton")) {
                frame->setStyleSheet(settingsCardStyleSheet(m_theme)
                                     + secondaryButtonStyleSheet(m_theme));
            } else {
                frame->setStyleSheet(settingsCardStyleSheet(m_theme));
            }
        }
        if (m_startupModeCombo) {
            m_startupModeCombo->setStyleSheet(comboBoxStyleSheet(m_theme));
        }
        if (m_deepseekApiKeyEdit) {
            m_deepseekApiKeyEdit->setStyleSheet(QString());
        }
        if (m_versionLabel) {
            m_versionLabel->setStyleSheet(QStringLiteral("color: %1;").arg(m_theme.textMuted));
        }
        for (QLabel *label : m_settingsContent->findChildren<QLabel *>()) {
            const QString name = label->objectName();
            if (name == QStringLiteral("settingsPageTitle")) {
                label->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 600; color: %1;")
                                         .arg(m_theme.textPrimary));
            } else if (name == QStringLiteral("settingsSectionTitle")) {
                label->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600; margin-top: 12px; color: %1;")
                                         .arg(m_theme.textPrimary));
            } else if (name == QStringLiteral("settingsMutedLabel")) {
                label->setStyleSheet(QStringLiteral("font-size: 14px; margin-top: 8px; color: %1;")
                                         .arg(m_theme.textSecondary));
            }
        }
    }
    if (m_aiReportPage) {
        m_aiReportPage->applyTheme(m_theme);
    }
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
    m_weeklyTopAppsByDay.clear();
    m_appIconByName.clear();
    QVector<int> dailySecondsByHour(24, 0);
    QMap<int, QHash<QString, int>> dailyAppSecondsByHour;
    QVector<QHash<QString, int>> weeklyAppSecondsByDay(7);
    QVector<int> weeklySecondsByDay(7, 0);

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
        if (!date.isValid()) continue;
        const int index = weekStart.daysTo(date);
        if (index < 0 || index >= 7) continue;

        weeklySecondsByDay[index] += record.durationSeconds;
        weeklyAppSecondsByDay[index][record.appName] += record.durationSeconds;
    }

    for (int i = 0; i < 7; ++i) {
        m_weeklyMinutesByDay[i] = (weeklySecondsByDay[i] + 59) / 60;
        QVector<QPair<QString, int>> topApps;
        for (auto it = weeklyAppSecondsByDay[i].cbegin(); it != weeklyAppSecondsByDay[i].cend(); ++it) {
            topApps.append({it.key(), (it.value() + 59) / 60});
        }
        std::sort(topApps.begin(), topApps.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
        if (topApps.size() > 3) {
            topApps.resize(3);
        }
        if (!topApps.isEmpty()) {
            m_weeklyTopAppsByDay[i] = topApps;
        }
    }

    for (int hour = 0; hour < dailySecondsByHour.size(); ++hour) {
        m_dailyMinutesByHour[hour] = dailySecondsByHour[hour] / 60;

        QVector<QPair<QString, int>> list;
        const auto appSeconds = dailyAppSecondsByHour.value(hour);
        for (auto it = appSeconds.cbegin(); it != appSeconds.cend(); ++it) {
            list.append({it.key(), it.value() / 60});
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

    for (auto it = dailyAppSeconds.cbegin(); it != dailyAppSeconds.cend(); ++it) {
        m_appIconByName[it.key()] = iconForApp(it.key());
    }
    for (auto it = weeklyAppSeconds.cbegin(); it != weeklyAppSeconds.cend(); ++it) {
        if (!m_appIconByName.contains(it.key())) {
            m_appIconByName[it.key()] = iconForApp(it.key());
        }
    }

    if (m_dailyButton && m_dailyButton->isChecked()) {
        fillAppStatsForDaily();
    } else {
        fillAppStatsForWeekly();
    }
}

void MainWindow::updateDailyChartAndSummary()
{
    if (!m_hourlyChartWidget || !m_primaryStatLabel) {
        return;
    }

    const int totalMinutes = std::accumulate(m_dailyMinutesByHour.cbegin(), m_dailyMinutesByHour.cend(), 0);
    m_primaryStatLabel->setText(QStringLiteral("今日总时长 %1").arg(formatDuration(totalMinutes * 60)));
    QStringList hourLabels;
    for (int i = 0; i < 24; ++i) {
        hourLabels.append(QStringLiteral("%1时").arg(i));
    }
    m_hourlyChartWidget->setChartData(m_dailyMinutesByHour, hourLabels, m_dailyTopAppsByHour, m_appIconByName, true, 60, 30);
}

void MainWindow::updateWeeklySummary()
{
    if (!m_hourlyChartWidget || !m_primaryStatLabel) {
        return;
    }

    const int totalMinutes = std::accumulate(m_weeklyMinutesByDay.cbegin(), m_weeklyMinutesByDay.cend(), 0);
    const int avgMinutes = m_weeklyMinutesByDay.isEmpty() ? 0 : totalMinutes / m_weeklyMinutesByDay.size();
    m_primaryStatLabel->setText(
        QStringLiteral("近7天总时长 %1        日均时长 %2")
            .arg(formatDuration(totalMinutes * 60), formatDuration(avgMinutes * 60)));

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

    m_hourlyChartWidget->setChartData(m_weeklyMinutesByDay, dayLabels, m_weeklyTopAppsByDay, m_appIconByName, false, dynamicMax, tick);
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
        runKey.setValue(key, QStringLiteral("\"%1\" --autostart").arg(appPath));
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

QString MainWindow::startupLaunchMode() const
{
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    return settings.value(QStringLiteral("startup/launch_mode"), QStringLiteral("tray")).toString();
}

void MainWindow::setStartupLaunchMode(const QString &mode) const
{
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    settings.setValue(QStringLiteral("startup/launch_mode"), mode);
}
void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);

    // 用齿轮 SVG 作为托盘图标（和设置按钮同款）
    const QIcon appIcon(QStringLiteral(":/icons/icons/app.png"));
    m_trayIcon->setIcon(appIcon);
    setWindowIcon(appIcon);

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
        if (m_homeButton) {
            m_homeButton->setFixedSize(176, 48);
            m_homeButton->setText(QStringLiteral("  主页"));
        }
        if (m_aiReportButton) {
            m_aiReportButton->setFixedSize(176, 48);
            m_aiReportButton->setText(QStringLiteral("  分析报告"));
        }
        const QString expandedStyle = navButtonStyleSheet(m_theme, true);
        m_settingsButton->setStyleSheet(expandedStyle);
        if (m_homeButton) {
            m_homeButton->setStyleSheet(expandedStyle);
            m_homeButton->setToolTip(QString());
        }
        if (m_aiReportButton) {
            m_aiReportButton->setStyleSheet(expandedStyle);
            m_aiReportButton->setToolTip(QString());
        }
        m_sidebarToggleButton->setText(QStringLiteral("☰"));
        m_sidebarToggleButton->setToolTip(QString());
        m_settingsButton->setToolTip(QString());
    } else {
        m_leftSidebar->setFixedWidth(68);
        m_settingsButton->setFixedSize(44, 44);
        m_settingsButton->setText(QString());
        const QString collapsedStyle = navButtonStyleSheet(m_theme, false);
        m_settingsButton->setStyleSheet(collapsedStyle);
        if (m_homeButton) {
            m_homeButton->setFixedSize(44, 44);
            m_homeButton->setText(QString());
            m_homeButton->setStyleSheet(collapsedStyle);
            m_homeButton->setToolTip(QStringLiteral("返回主页"));
        }
        if (m_aiReportButton) {
            m_aiReportButton->setFixedSize(44, 44);
            m_aiReportButton->setText(QString());
            m_aiReportButton->setStyleSheet(collapsedStyle);
            m_aiReportButton->setToolTip(QStringLiteral("分析报告"));
        }
        m_sidebarToggleButton->setText(QStringLiteral("≡"));
        m_sidebarToggleButton->setToolTip(QStringLiteral("展开菜单"));
        m_settingsButton->setToolTip(QStringLiteral("设置和帮助"));
    }
}

void MainWindow::clampUsageSplitter()
{
    if (!m_usageSplitter || m_usageSplitter->count() < 2) {
        return;
    }

    QList<int> sizes = m_usageSplitter->sizes();
    if (sizes.size() < 2) {
        return;
    }

    const int total = sizes[0] + sizes[1];
    const int minTop = 56;
    const int minBottom = 56;
    const int maxTop = qMax(minTop, total - minBottom);
    const int clampedTop = qBound(minTop, sizes[0], maxTop);
    const int clampedBottom = total - clampedTop;

    if (clampedTop != sizes[0]) {
        m_usageSplitter->setSizes({clampedTop, clampedBottom});
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_usageSplitter && watched == m_usageSplitter->handle(1)) {
        if (event->type() == QEvent::Enter) {
            if (auto *handle = qobject_cast<QWidget *>(watched)) {
                handle->setCursor(Qt::SizeVerCursor);
                QToolTip::showText(handle->mapToGlobal(QPoint(handle->width() / 2, 0)),
                                   QStringLiteral("拖动调整柱状图和应用统计区域大小"),
                                   handle);
            }
        } else if (event->type() == QEvent::Leave) {
            QToolTip::hideText();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
