#include "mainwindow.h"
#include "core/database.h"
#include "core/startupmanager.h"
#include "hourlychartwidget.h"
#include "windowblur.h"
#include "aireportpage.h"
#include "fluenttoggleswitch.h"
#include "theme/thememanager.h"
#include "theme/themestyles.h"
#include "i18n/translationmanager.h"
#include <QDebug>
#include <QApplication>
#include <QGuiApplication>
#include <QShowEvent>
#include <QStyleHints>
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
#include <QSlider>
#include <QVBoxLayout>
#include <QVariant>
#include <QLineEdit>
#include <QWidget>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QProcess>
#include <QApplication>
#include <QEasingCurve>

#ifndef SCREENTIME_VERSION
#define SCREENTIME_VERSION "2.0"
#endif

namespace {

// 启动阶段日志（由 main.cpp 安装的消息处理器写入 startup.log）
void stage(const char *name)
{
    qInfo().noquote() << (QStringLiteral("[stage] ") + QString::fromLatin1(name));
}

// 自绘图标 SVG 模板：颜色用 currentColor 占位，渲染时由当前主题色填充，
// 保证绘图代码里不再出现具体颜色值。
const char *kHomeIconSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="currentColor" d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z"/>
</svg>
)SVG";

const char *kAiIconSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="currentColor" d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8l-6-6zm-1 2.5L18.5 10H13V4.5zM8 12h8v1.5H8V12zm0 3h8v1.5H8V15zm0 3h5v1.5H8V18z"/>
</svg>
)SVG";

const char *kGearIconSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="currentColor" d="M19.14 12.94c.04-.31.06-.63.06-.94s-.02-.63-.06-.94l2.03-1.58a.5.5 0 0 0 .12-.64l-1.92-3.32a.5.5 0 0 0-.6-.22l-2.39.96a7.1 7.1 0 0 0-1.63-.94L14.4 2.8a.5.5 0 0 0-.49-.4h-3.84a.5.5 0 0 0-.49.4l-.36 2.52c-.58.23-1.12.54-1.63.94l-2.39-.96a.5.5 0 0 0-.6.22L2.68 8.84a.5.5 0 0 0 .12.64l2.03 1.58c-.04.31-.06.63-.06.94s.02.63.06.94L2.8 14.52a.5.5 0 0 0-.12.64l1.92 3.32c.13.23.4.32.64.22l2.39-.96c.5.4 1.05.72 1.63.94l.36 2.52c.04.24.25.4.49.4h3.84c.24 0 .45-.16.49-.4l.36-2.52c.58-.23 1.12-.54 1.63-.94l2.39.96c.24.1.51.01.64-.22l1.92-3.32a.5.5 0 0 0-.12-.64l-2.03-1.58ZM12 15.5A3.5 3.5 0 1 1 12 8.5a3.5 3.5 0 0 1 0 7Z"/>
</svg>
)SVG";

const char *kShareIconSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="currentColor" d="M14 3a1 1 0 1 0 0 2h3.59l-7.3 7.3a1 1 0 1 0 1.42 1.4L19 6.41V10a1 1 0 1 0 2 0V4a1 1 0 0 0-1-1h-6Z"/>
  <path fill="currentColor" d="M6 5a3 3 0 0 0-3 3v10a3 3 0 0 0 3 3h10a3 3 0 0 0 3-3v-5a1 1 0 1 0-2 0v5a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V8a1 1 0 0 1 1-1h5a1 1 0 1 0 0-2H6Z"/>
</svg>
)SVG";

// 汉堡菜单：三条横线，用于侧边栏展开/折叠按钮。
const char *kMenuIconSvg = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <path fill="currentColor" d="M4 6h16v2.2H4V6zm0 4.9h16v2.2H4v-2.2zm0 4.9h16V18H4v-2.2z"/>
</svg>
)SVG";

// 应用统计的现代列表行：图标 + 名称 + 细进度条 + 时长。
// 不再用逐项圆角卡片，分组靠列表自身的细分隔线。
class AppStatRow : public QWidget
{
public:
    AppStatRow(const QIcon &icon, const QString &name, const QString &durationText,
               int seconds, int maxSeconds, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_icon(icon)
        , m_name(name)
        , m_durationText(durationText)
        , m_seconds(seconds)
        , m_maxSeconds(qMax(1, maxSeconds))
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setMinimumHeight(52);
        // 颜色全部来自当前 Theme，主题切换时自动重绘。
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
                [this](const Theme &) { update(); });
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const ColorTokens &c = ThemeManager::instance().theme().colors;
        const QRect r = rect().adjusted(14, 0, -14, 0);
        const int iconSize = 20;
        const QRect iconRect(r.left(), r.center().y() - iconSize / 2, iconSize, iconSize);
        if (!m_icon.isNull()) {
            m_icon.paint(&p, iconRect, Qt::AlignCenter, QIcon::Normal);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(c.chart.placeholderIcon);
            p.drawRoundedRect(iconRect, 5, 5);
        }

        const int textLeft = iconRect.right() + 12;
        const int rightWidth = 92;
        const int contentRight = r.right() - rightWidth;

        p.setPen(c.textPrimary);
        const QRect nameRect(textLeft, r.top() + 10, qMax(10, contentRight - textLeft), 18);
        p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                   p.fontMetrics().elidedText(m_name, Qt::ElideRight, nameRect.width()));

        p.setPen(c.textSecondary);
        const QRect durRect(contentRight + 8, r.top() + 10, rightWidth - 8, 18);
        p.drawText(durRect, Qt::AlignRight | Qt::AlignVCenter, m_durationText);

        // 细进度条：纯色，颜色来自主题图表 Token。
        const int barTop = r.top() + 33;
        const QRect barBg(textLeft, barTop, qMax(10, r.right() - textLeft), 4);
        p.setPen(Qt::NoPen);
        p.setBrush(c.chart.barTrack);
        p.drawRoundedRect(barBg, 2, 2);
        const int filled = qBound(0, int(barBg.width() * double(m_seconds) / double(m_maxSeconds)), barBg.width());
        if (filled > 0) {
            p.setBrush(c.chart.bar);
            p.drawRoundedRect(QRect(barBg.left(), barBg.top(), qMax(3, filled), barBg.height()), 2, 2);
        }
    }

private:
    QIcon m_icon;
    QString m_name;
    QString m_durationText;
    int m_seconds;
    int m_maxSeconds;
};

} // namespace

MainWindow::MainWindow(Database *database, QWidget *parent)
    : QMainWindow(parent)
    , m_database(database)
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralWidget"));
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_leftSidebar = new QWidget(central);
    m_leftSidebar->setObjectName(QStringLiteral("sidebar"));
    m_leftSidebar->setFixedWidth(190);
    auto *sidebarLayout = new QVBoxLayout(m_leftSidebar);
    sidebarLayout->setContentsMargins(8, 8, 8, 8);
    sidebarLayout->setSpacing(8);

    m_sidebarToggleButton = new QPushButton(m_leftSidebar);
    m_sidebarToggleButton->setObjectName(QStringLiteral("sidebarToggle"));
    m_sidebarToggleButton->setFixedSize(40, 40);
    m_sidebarToggleButton->setCursor(Qt::PointingHandCursor);
    i18nSetToolTip(m_sidebarToggleButton, "tooltip.expand");
    sidebarLayout->addWidget(m_sidebarToggleButton, 0, Qt::AlignLeft | Qt::AlignTop);

    // --- 主页按钮（最上方） ---
    m_homeButton = new QPushButton(m_leftSidebar);
    m_homeButton->setObjectName(QStringLiteral("navButton"));
    m_homeButton->setFixedSize(174, 44);
    m_homeButton->setCheckable(true);
    m_homeButton->setText(QStringLiteral("  ") + i18n("nav.home"));
    sidebarLayout->addWidget(m_homeButton, 0, Qt::AlignLeft);

    connect(m_homeButton, &QPushButton::clicked, this, [this]() {
        m_homeButton->setChecked(true);
        if (m_aiReportButton) m_aiReportButton->setChecked(false);
        if (m_settingsButton) m_settingsButton->setChecked(false);
        m_contentStack->setCurrentIndex(0);
    });

    // --- 分析报告按钮（主页下方） ---
    m_aiReportButton = new QPushButton(m_leftSidebar);
    m_aiReportButton->setObjectName(QStringLiteral("navButton"));
    m_aiReportButton->setFixedSize(174, 44);
    m_aiReportButton->setCheckable(true);
    m_aiReportButton->setText(QStringLiteral("  ") + i18n("nav.report"));
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
    m_settingsButton->setObjectName(QStringLiteral("navButton"));
    m_settingsButton->setFixedSize(176, 48);
    m_settingsButton->setCheckable(true);
    m_settingsButton->setText(QStringLiteral("  ") + i18n("nav.settings"));
    sidebarLayout->addWidget(m_settingsButton, 0, Qt::AlignLeft | Qt::AlignBottom);

    connect(m_settingsButton, &QPushButton::clicked, this, [this]() {
        m_settingsButton->setChecked(true);
        if (m_homeButton) m_homeButton->setChecked(false);
        if (m_aiReportButton) m_aiReportButton->setChecked(false);
        m_contentStack->setCurrentIndex(1);
    });

    m_contentStack = new QStackedWidget(central);
    m_contentStack->setMinimumSize(0, 0);
    stage("ctor: createUsagePage");
    m_contentStack->addWidget(createUsagePage());
    stage("ctor: createUsagePage done");
    stage("ctor: createSettingsPage");
    m_contentStack->addWidget(createSettingsPage());
    stage("ctor: createSettingsPage done");

    // 创建分析报告页面（须在 createSettingsPage 之后，以便同步已保存的配置）
    stage("ctor: AIReportPage");
    m_aiReportPage = new AIReportPage(m_database, central);
    m_contentStack->addWidget(m_aiReportPage);
    syncAIReportSettings();
    stage("ctor: AIReportPage done");

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
    stage("ctor: setupTrayIcon");
    setupTrayIcon();
    stage("ctor: setupTrayIcon done");

    // 主题解耦：ThemeManager 负责当前主题与切换，MainWindow 只响应刷新信号。
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            &MainWindow::onThemeChanged);
    // 语言切换：重设所有界面文本。
    connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this,
            [this]() { retranslateUi(); });
    stage("ctor: onThemeChanged");
    onThemeChanged(ThemeManager::instance().theme());
    stage("ctor: onThemeChanged done");
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // 原生标题栏属性需要有效 HWND，显示后再同步一次。
    WindowBlur::applyFrame(this, m_theme.dark);
}

QWidget *MainWindow::createUsagePage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto *switchRow = new QHBoxLayout();
    m_dailyButton = new QPushButton(page);
    m_weeklyButton = new QPushButton(page);
    i18nSetText(m_dailyButton, "period.daily");
    i18nSetText(m_weeklyButton, "period.weekly");
    m_dailyButton->setObjectName(QStringLiteral("periodButton"));
    m_weeklyButton->setObjectName(QStringLiteral("periodButton"));
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
    chartPanel->setObjectName(QStringLiteral("chartCard"));
    chartPanel->setFrameShape(QFrame::StyledPanel);
    auto *chartLayout = new QVBoxLayout(chartPanel);
    chartLayout->setContentsMargins(10, 10, 10, 10);
    chartLayout->setSpacing(6);
    m_primaryStatLabel = new QLabel(chartPanel);
    m_primaryStatLabel->setObjectName(QStringLiteral("primaryStatLabel"));
    m_hourlyChartWidget = new HourlyChartWidget(chartPanel);
    m_hourlyChartWidget->setMinimumHeight(0);
    m_hourlyChartWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartLayout->addWidget(m_primaryStatLabel);
    chartLayout->addWidget(m_hourlyChartWidget, 1);

    auto *statsPanel = new QFrame(page);
    statsPanel->setObjectName(QStringLiteral("statsCard"));
    statsPanel->setFrameShape(QFrame::NoFrame);
    auto *statsLayout = new QVBoxLayout(statsPanel);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setSpacing(8);
    auto *statsTitle = new QLabel(statsPanel);
    statsTitle->setObjectName(QStringLiteral("statsTitle"));
    i18nSetText(statsTitle, "stats.title");
    m_statsTitleLabel = statsTitle;
    m_appStatsList = new QListWidget(statsPanel);
    m_appStatsList->setObjectName(QStringLiteral("appStatsList"));
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
        i18nSetToolTip(handle, "tooltip.splitter");
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
    stage("usage: refreshUsageData");
    refreshUsageData();
    stage("usage: refreshUsageData done");
    fillAppStatsForDaily();
    return page;
}

QWidget *MainWindow::createSettingsPage()
{
    stage("settings: begin");
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
    scrollArea->setMinimumSize(0, 0);
    content->setMinimumSize(0, 0);

    auto *wrapper = new QWidget(content);
    wrapper->setMaximumWidth(780);
    auto *wrapperLayout = new QVBoxLayout(wrapper);
    wrapperLayout->setContentsMargins(4, 0, 4, 0);
    wrapperLayout->setSpacing(10);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(32, 28, 32, 32);
    layout->setSpacing(0);
    layout->addWidget(wrapper, 1);

    auto *title = new QLabel(wrapper);
    title->setObjectName(QStringLiteral("settingsPageTitle"));
    i18nSetText(title, "settings.title");
    title->setContentsMargins(0, 4, 0, 4);

    auto *themeRow = new QFrame(wrapper);
    themeRow->setObjectName(QStringLiteral("settingsCard"));
    auto *themeLayout = new QHBoxLayout(themeRow);
    themeLayout->setContentsMargins(14, 12, 14, 12);
    themeLayout->setSpacing(12);
    auto *themeLabel = new QLabel(themeRow);
    i18nSetText(themeLabel, "settings.theme");
    m_themeCombo = new QComboBox(themeRow);
    rebuildThemeCombo();
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_themeCombo || index < 0) {
            return;
        }
        ThemeManager::instance().setTheme(m_themeCombo->itemData(index).toString());
    });
    themeLayout->addWidget(themeLabel);
    themeLayout->addStretch();
    themeLayout->addWidget(m_themeCombo);
    wrapperLayout->addWidget(title);

    // ── 界面语言（标签固定为英文 "Language"）──────────────────
    auto *languageRow = new QFrame(wrapper);
    languageRow->setObjectName(QStringLiteral("settingsCard"));
    auto *languageLayout = new QHBoxLayout(languageRow);
    languageLayout->setContentsMargins(14, 12, 14, 12);
    languageLayout->setSpacing(12);
    auto *languageLabel = new QLabel(QStringLiteral("Language"), languageRow);
    m_languageCombo = new QComboBox(languageRow);
    rebuildLanguageCombo();
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_languageCombo || index < 0) {
            return;
        }
        TranslationManager::instance().setLanguage(m_languageCombo->itemData(index).toString());
    });
    languageLayout->addWidget(languageLabel);
    languageLayout->addStretch();
    languageLayout->addWidget(m_languageCombo);
    wrapperLayout->addWidget(languageRow);
    wrapperLayout->addWidget(themeRow);

        // ── 强调色选择（下拉框）────────────────────────────────
    auto *accentRow = new QFrame(wrapper);
    accentRow->setObjectName(QStringLiteral("settingsCard"));
    auto *accentLayout = new QHBoxLayout(accentRow);
    accentLayout->setContentsMargins(14, 12, 14, 12);
    accentLayout->setSpacing(12);
    auto *accentLabel = new QLabel(accentRow);
    i18nSetText(accentLabel, "settings.accent");
    m_accentCombo = new QComboBox(accentRow);
    rebuildAccentCombo();
    connect(m_accentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_accentCombo || index < 0) return;
        const AccentColor newAccent = static_cast<AccentColor>(m_accentCombo->itemData(index).toInt());
        ThemeManager::instance().setAccent(newAccent);
    });
    accentLayout->addWidget(accentLabel);
    accentLayout->addStretch();
    accentLayout->addWidget(m_accentCombo);
        wrapperLayout->addWidget(accentRow);

        // ── 允许通知 ────────────────────────────────────────────
    auto *notifRow = new QFrame(wrapper);
    notifRow->setObjectName(QStringLiteral("settingsCard"));
    auto *notifLayout = new QHBoxLayout(notifRow);
    notifLayout->setContentsMargins(14, 12, 14, 12);
    notifLayout->setSpacing(12);
    auto *notifLabel = new QLabel(notifRow);
    i18nSetText(notifLabel, "settings.notifications");
    m_notificationSwitch = new FluentToggleSwitch(notifRow);
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const bool notifEnabled = settings.value(QStringLiteral("ui/notifications"), true).toBool();
    m_notificationSwitch->setChecked(notifEnabled);
    connect(m_notificationSwitch, &FluentToggleSwitch::toggled, this, [](bool checked) {
        QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
        s.setValue(QStringLiteral("ui/notifications"), checked);
    });
    notifLayout->addWidget(notifLabel);
    notifLayout->addStretch();
    notifLayout->addWidget(m_notificationSwitch);
    wrapperLayout->addWidget(notifRow);

    // ── 开机自启动 ──────────────────────────────────────────
    auto *autoStartRow = new QFrame(wrapper);
    autoStartRow->setObjectName(QStringLiteral("settingsCard"));
    auto *rowLayout = new QHBoxLayout(autoStartRow);
    rowLayout->setContentsMargins(14, 12, 14, 12);
    rowLayout->setSpacing(12);

    auto *label = new QLabel(autoStartRow);
    i18nSetText(label, "settings.autostart");
        m_autoStartSwitch = new FluentToggleSwitch(autoStartRow);

        rowLayout->addWidget(label);
        rowLayout->addStretch();
        rowLayout->addWidget(m_autoStartSwitch);

        stage("settings: query autostart");
        const bool enabled = isAutoStartEnabled();
        stage("settings: autostart queried");
        m_autoStartSwitch->setChecked(enabled);
        connect(m_autoStartSwitch, &FluentToggleSwitch::toggled, this, [this](bool checked) {
        if (!setAutoStartEnabled(checked) && m_autoStartSwitch) {
            m_autoStartSwitch->blockSignals(true);
            m_autoStartSwitch->setChecked(!checked);
            m_autoStartSwitch->blockSignals(false);
            if (checked) {
                // MSIX 下开机自启动由系统 startupTask 托管，一旦被用户在
                // “任务管理器 → 启动应用”或组策略里禁用，程序无法自行开启。
                QMessageBox::information(this, i18n("autostart.failed_title"),
                    i18n("autostart.failed_body"));
            }
        }
    });

    wrapperLayout->addWidget(autoStartRow);

    auto *startupModeRow = new QFrame(wrapper);
    startupModeRow->setObjectName(QStringLiteral("settingsCard"));
    auto *modeLayout = new QHBoxLayout(startupModeRow);
    modeLayout->setContentsMargins(14, 12, 14, 12);
    modeLayout->setSpacing(12);

    auto *modeLabel = new QLabel(startupModeRow);
    i18nSetText(modeLabel, "settings.autostart_mode");
    m_startupModeCombo = new QComboBox(startupModeRow);
    rebuildStartupModeCombo();

    const QString mode = startupLaunchMode();
    const int modeIndex = qMax(0, m_startupModeCombo->findData(mode));
    m_startupModeCombo->blockSignals(true);
    m_startupModeCombo->setCurrentIndex(modeIndex);
    m_startupModeCombo->blockSignals(false);
    connect(m_startupModeCombo, &QComboBox::currentIndexChanged, this, [this]() {
        if (!m_startupModeCombo) {
            return;
        }
        setStartupLaunchMode(m_startupModeCombo->currentData().toString());
    });

    modeLayout->addWidget(modeLabel);
    modeLayout->addStretch();
    modeLayout->addWidget(m_startupModeCombo);

    wrapperLayout->addWidget(startupModeRow);

    auto *githubRow = new QFrame(wrapper);
    githubRow->setObjectName(QStringLiteral("settingsCard"));
    githubRow->setProperty("settingsCardVariant", QStringLiteral("link"));
    auto *githubLayout = new QHBoxLayout(githubRow);
    githubLayout->setContentsMargins(14, 12, 14, 12);
    githubLayout->setSpacing(10);

    auto *githubLabel = new QLabel(githubRow);
    i18nSetText(githubLabel, "settings.repo");
    auto *githubButton = new QPushButton(QStringLiteral("Yiuk-max/Screen-Time"), githubRow);
    githubButton->setCursor(Qt::PointingHandCursor);
    connect(githubButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Yiuk-max/Screen-Time")));
    });

    m_shareButton = new QToolButton(githubRow);
    m_shareButton->setIconSize(QSize(18, 18));
    m_shareButton->setAutoRaise(true);
    i18nSetToolTip(m_shareButton, "tooltip.open_link");
    m_shareButton->setCursor(Qt::PointingHandCursor);
    connect(m_shareButton, &QToolButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Yiuk-max/Screen-Time")));
    });

    githubLayout->addWidget(githubLabel);
    githubLayout->addSpacing(8);
    githubLayout->addWidget(githubButton, 1);
    githubLayout->addWidget(m_shareButton, 0, Qt::AlignRight);

    wrapperLayout->addWidget(githubRow);

    // ========== 分析报告配置区域（独立栏目） ==========
    auto *aiTitle = new QLabel(wrapper);
    aiTitle->setObjectName(QStringLiteral("settingsSectionTitle"));
    i18nSetText(aiTitle, "settings.ai_section");

    // 启用开关
    auto *aiEnableRow = new QFrame(wrapper);
    aiEnableRow->setObjectName(QStringLiteral("settingsCard"));
    auto *aiEnableLayout = new QHBoxLayout(aiEnableRow);
    aiEnableLayout->setContentsMargins(14, 12, 14, 12);
    aiEnableLayout->setSpacing(12);

    auto *aiEnableLabel = new QLabel(aiEnableRow);
    i18nSetText(aiEnableLabel, "settings.ai_enable");
        m_aiReportEnabledSwitch = new FluentToggleSwitch(aiEnableRow);

    aiEnableLayout->addWidget(aiEnableLabel);
    aiEnableLayout->addStretch();
    aiEnableLayout->addWidget(m_aiReportEnabledSwitch);

    wrapperLayout->addWidget(aiTitle);
    wrapperLayout->addWidget(aiEnableRow);

    // DeepSeek API Key 输入
    auto *apiKeyRow = new QFrame(wrapper);
    apiKeyRow->setObjectName(QStringLiteral("settingsCard"));
    apiKeyRow->setProperty("settingsCardVariant", QStringLiteral("lineEdit"));
    auto *apiKeyLayout = new QHBoxLayout(apiKeyRow);
    apiKeyLayout->setContentsMargins(14, 12, 14, 12);
    apiKeyLayout->setSpacing(12);

    auto *apiKeyLabel = new QLabel(apiKeyRow);
    i18nSetText(apiKeyLabel, "settings.api_key");
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

    wrapperLayout->addWidget(apiKeyRow);

    // 连接启用开关
    connect(m_aiReportEnabledSwitch, &FluentToggleSwitch::toggled, this, [this](bool checked) {
        QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
        s.setValue(QStringLiteral("ai/enabled"), checked);
        syncAIReportSettings();
    });

    // 加载保存的启用状态（AI 页面创建后由 syncAIReportSettings 同步）
    QSettings aiToggledSettings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const bool aiEnabled = aiToggledSettings.value(QStringLiteral("ai/enabled"), false).toBool();
    m_aiReportEnabledSwitch->setChecked(aiEnabled);

    // ========== 关于 ==========
    auto *aboutTitle = new QLabel(wrapper);
    aboutTitle->setObjectName(QStringLiteral("settingsSectionTitle"));
    i18nSetText(aboutTitle, "settings.help");

    auto *versionRow = new QFrame(wrapper);
    versionRow->setObjectName(QStringLiteral("settingsCard"));
    auto *versionLayout = new QHBoxLayout(versionRow);
    versionLayout->setContentsMargins(14, 12, 14, 12);
    versionLayout->setSpacing(12);

    auto *versionLabel = new QLabel(versionRow);
    i18nSetText(versionLabel, "settings.version");
    m_versionLabel = new QLabel(QStringLiteral(SCREENTIME_VERSION), versionRow);
    m_versionLabel->setObjectName(QStringLiteral("mutedValue"));

    versionLayout->addWidget(versionLabel);
    versionLayout->addStretch();
    versionLayout->addWidget(m_versionLabel);

    wrapperLayout->addWidget(aboutTitle);

    auto makeHelpRow = [&](const char *labelKey, const QString &buttonText,
                           const char *buttonKey, const auto &handler) {
        auto *row = new QFrame(wrapper);
        row->setObjectName(QStringLiteral("settingsCard"));
        row->setProperty("settingsCardVariant", QStringLiteral("link"));
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(14, 12, 14, 12);
        rowLayout->setSpacing(10);

        auto *label = new QLabel(row);
        i18nSetText(label, labelKey);
        auto *button = new QPushButton(buttonText, row);
        if (buttonKey) {
            i18nSetText(button, buttonKey);
        }
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, handler);

        rowLayout->addWidget(label);
        rowLayout->addStretch();
        rowLayout->addWidget(button);
        wrapperLayout->addWidget(row);
    };

    makeHelpRow("settings.about", QStringLiteral("Screen Time %1").arg(QStringLiteral(SCREENTIME_VERSION)), nullptr, [this]() {
        QMessageBox::about(this, i18n("about.title"),
                           i18n("about.body").arg(QStringLiteral(SCREENTIME_VERSION)));
    });
    makeHelpRow("settings.repo", QStringLiteral("Yiuk-max/Screen-Time"), nullptr, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Yiuk-max/Screen-Time")));
    });

    wrapperLayout->addWidget(versionRow);
    layout->addStretch();

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);
    stage("settings: end");
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

void MainWindow::onThemeChanged(const Theme &theme)
{
    m_theme = theme;

    // 同步原生标题栏深浅色。
    WindowBlur::applyFrame(this, m_theme.dark);

    // 程序化切换主题时同步下拉框选中项（不触发再次切换）。
    if (m_themeCombo) {
        const int index = m_themeCombo->findData(ThemeManager::instance().themeId());
        if (index >= 0 && index != m_themeCombo->currentIndex()) {
            m_themeCombo->blockSignals(true);
            m_themeCombo->setCurrentIndex(index);
            m_themeCombo->blockSignals(false);
        }
    }
    if (m_accentCombo) {
        const int index = m_accentCombo->findData(static_cast<int>(ThemeManager::instance().accent()));
        if (index >= 0 && index != m_accentCombo->currentIndex()) {
            m_accentCombo->blockSignals(true);
            m_accentCombo->setCurrentIndex(index);
            m_accentCombo->blockSignals(false);
        }
    }

    // 图标颜色需要按主题重新渲染。
    refreshIcons(theme);
}

void MainWindow::refreshIcons(const Theme &theme)
{
    const QSize iconSize(theme.design.iconSize, theme.design.iconSize);
    const QColor navColor = theme.colors.textSecondary;

    if (m_homeButton) {
        m_homeButton->setIcon(ThemeStyles::svgIcon(QString::fromUtf8(kHomeIconSvg), navColor, iconSize));
        m_homeButton->setIconSize(iconSize);
    }
    if (m_aiReportButton) {
        m_aiReportButton->setIcon(ThemeStyles::svgIcon(QString::fromUtf8(kAiIconSvg), navColor, iconSize));
        m_aiReportButton->setIconSize(iconSize);
    }
    if (m_settingsButton) {
        m_settingsButton->setIcon(ThemeStyles::svgIcon(QString::fromUtf8(kGearIconSvg), navColor, iconSize));
        m_settingsButton->setIconSize(iconSize);
    }
    if (m_shareButton) {
        m_shareButton->setIcon(ThemeStyles::svgIcon(QString::fromUtf8(kShareIconSvg), theme.colors.link, iconSize));
        m_shareButton->setIconSize(iconSize);
    }
    if (m_sidebarToggleButton) {
        m_sidebarToggleButton->setIcon(
            ThemeStyles::svgIcon(QString::fromUtf8(kMenuIconSvg), theme.colors.textPrimary, QSize(20, 20)));
        m_sidebarToggleButton->setIconSize(QSize(20, 20));
    }
}

void MainWindow::rebuildThemeCombo()
{
    if (!m_themeCombo) {
        return;
    }
    const QString current = ThemeManager::instance().themeId();
    m_themeCombo->blockSignals(true);
    m_themeCombo->clear();
    for (const ThemeOption &option : ThemeManager::instance().themeOptions()) {
        const QString key = QStringLiteral("theme.") + option.id;
        const QString name = TranslationManager::instance().has(key) ? i18n(key) : option.name;
        m_themeCombo->addItem(name, option.id);
        const QString descKey = key + QStringLiteral(".desc");
        const QString description =
            TranslationManager::instance().has(descKey) ? i18n(descKey) : option.description;
        if (!description.isEmpty()) {
            m_themeCombo->setItemData(m_themeCombo->count() - 1, description, Qt::ToolTipRole);
        }
    }
    const int index = m_themeCombo->findData(current);
    m_themeCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_themeCombo->blockSignals(false);
}

void MainWindow::rebuildAccentCombo()
{
    if (!m_accentCombo) {
        return;
    }
    const int current = static_cast<int>(ThemeManager::instance().accent());
    m_accentCombo->blockSignals(true);
    m_accentCombo->clear();
    for (const AccentOption &option : ThemeManager::instance().accentOptions()) {
        const QString key = QStringLiteral("accent.") + option.id;
        const QString name = TranslationManager::instance().has(key) ? i18n(key) : option.name;
        m_accentCombo->addItem(name, static_cast<int>(option.value));
    }
    const int index = m_accentCombo->findData(current);
    m_accentCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_accentCombo->blockSignals(false);
}

void MainWindow::rebuildStartupModeCombo()
{
    if (!m_startupModeCombo) {
        return;
    }
    const QString current = m_startupModeCombo->currentData().toString();
    m_startupModeCombo->blockSignals(true);
    m_startupModeCombo->clear();
    m_startupModeCombo->addItem(i18n("startup.tray"), QStringLiteral("tray"));
    m_startupModeCombo->addItem(i18n("startup.window"), QStringLiteral("window"));
    const int index = m_startupModeCombo->findData(current);
    m_startupModeCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_startupModeCombo->blockSignals(false);
}

void MainWindow::rebuildLanguageCombo()
{
    if (!m_languageCombo) {
        return;
    }
    const QString current = TranslationManager::instance().languageId();
    m_languageCombo->blockSignals(true);
    m_languageCombo->clear();
    for (const LanguageInfo &info : TranslationManager::instance().languages()) {
        m_languageCombo->addItem(info.nativeName, info.id);
    }
    const int index = m_languageCombo->findData(current);
    m_languageCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_languageCombo->blockSignals(false);
}

void MainWindow::retranslateUi()
{
    i18nRetranslate(this);
    rebuildThemeCombo();
    rebuildAccentCombo();
    rebuildStartupModeCombo();
    rebuildLanguageCombo();
    if (m_showAction) {
        m_showAction->setText(i18n("tray.show"));
    }
    if (m_quitAction) {
        m_quitAction->setText(i18n("tray.quit"));
    }
    applySidebarMode(m_sidebarExpanded);
    if (m_dailyButton && m_dailyButton->isChecked()) {
        updateDailyChartAndSummary();
    } else {
        updateWeeklySummary();
    }
    if (m_aiReportPage) {
        m_aiReportPage->retranslateUi();
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
    if (sortedItems.isEmpty()) {
        return;
    }

    const int maxSeconds = qMax(1, sortedItems.first().second);
    for (const auto &item : sortedItems) {
        auto *row = new QListWidgetItem(m_appStatsList);
        row->setSizeHint(QSize(0, 52));
        row->setFlags(Qt::ItemIsEnabled);
        auto *widget = new AppStatRow(iconForApp(item.first), item.first,
                                      formatDuration(item.second),
                                      item.second, maxSeconds, m_appStatsList);
        m_appStatsList->setItemWidget(row, widget);
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
    m_primaryStatLabel->setText(i18n("stat.today_total").arg(formatDuration(totalMinutes * 60)));
    QStringList hourLabels;
    for (int i = 0; i < 24; ++i) {
        hourLabels.append(i18n("hour.label").arg(i));
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
        i18n("stat.week_total")
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
        return i18n("duration.hm").arg(hours).arg(minutes);
    }
    return i18n("duration.m").arg(minutes);
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
    return StartupManager::isAutoStartEnabled();
}

bool MainWindow::setAutoStartEnabled(bool enabled) const
{
    return StartupManager::setAutoStartEnabled(enabled);
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

    // 右键菜单（样式完全由全局主题提供）
    m_trayMenu = new QMenu(this);

    m_showAction = m_trayMenu->addAction(i18n("tray.show"));
    m_trayMenu->addSeparator();
    m_quitAction = m_trayMenu->addAction(i18n("tray.quit"));

    m_trayIcon->setContextMenu(m_trayMenu);
    ensureTrayIconVisible();

    // 登录后 Explorer / Shell_TrayWnd 可能比 startupTask 晚数秒创建。
    // QSystemTrayIcon 在此期间 show() 不一定会自动补注册，因此周期重试；
    // 最长约两分钟，覆盖 Windows 10 登录阶段的慢机器。
    m_trayRetryTimer.setInterval(2000);
    connect(&m_trayRetryTimer, &QTimer::timeout, this, &MainWindow::ensureTrayIconVisible);
    if (!m_trayIcon->isVisible() || !QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayRetryTimer.start();
    }

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

    connect(m_showAction, &QAction::triggered, this, [this]() {
        show();
        raise();
        activateWindow();
    });

    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::ensureTrayIconVisible()
{
    if (!m_trayIcon) {
        return;
    }

    ++m_trayRetryAttempts;
    if (m_trayRetryAttempts > 1) {
        // 强制 Qt 重新发送 NIM_ADD，避免第一次 show() 发生在
        // Shell_TrayWnd 尚未创建时只留下“逻辑可见”状态。
        m_trayIcon->hide();
    }
    m_trayIcon->show();

    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    if ((trayAvailable && m_trayIcon->isVisible()) || m_trayRetryAttempts >= 60) {
        m_trayRetryTimer.stop();
    }
}

void MainWindow::hideToTray()
{
    hide();
    if (!m_trayIcon) {
        return;
    }
    QSettings s(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    if (s.value(QStringLiteral("ui/notifications"), true).toBool()) {
        m_trayIcon->showMessage(
            QStringLiteral("Screen Time"),
            i18n("close.minimized"),
            QSystemTrayIcon::Information,
            2000);
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    // 点击最小化按钮时隐藏到系统托盘，而不是留在任务栏。
    if (event->type() == QEvent::WindowStateChange && isMinimized()) {
        // 延迟到事件处理结束后再隐藏，避免与最小化动画冲突。
        QTimer::singleShot(0, this, [this]() {
            Qt::WindowStates state = windowState();
            state.setFlag(Qt::WindowMinimized, false);
            setWindowState(state);
            hideToTray();
        });
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 点击关闭按钮时最小化到托盘，而不是退出
    event->ignore();
    hideToTray();
}

void MainWindow::applySidebarMode(bool expanded)
{
    m_sidebarExpanded = expanded;
    if (!m_leftSidebar || !m_settingsButton || !m_sidebarToggleButton) {
        return;
    }

    const DesignTokens design = ThemeManager::instance().theme().design;
    const int targetWidth = expanded ? design.sidebarExpandedWidth : design.sidebarCollapsedWidth;

    // Animate sidebar width with OutCubic easing
    auto *anim = new QPropertyAnimation(m_leftSidebar, "minimumWidth", this);
    anim->setDuration(design.animNormalMs);
    anim->setStartValue(m_leftSidebar->width());
    anim->setEndValue(targetWidth);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    auto *animMax = new QPropertyAnimation(m_leftSidebar, "maximumWidth", this);
    animMax->setDuration(design.animNormalMs);
    animMax->setStartValue(m_leftSidebar->maximumWidth());
    animMax->setEndValue(targetWidth);
    animMax->setEasingCurve(QEasingCurve::OutCubic);

    connect(anim, &QPropertyAnimation::finished, this, [this, expanded, design]() {
        m_leftSidebar->setFixedWidth(expanded ? design.sidebarExpandedWidth : design.sidebarCollapsedWidth);

        const int expandedW = design.navButtonWidthExpanded;
        const int collapsedW = design.navButtonWidthCollapsed;
        const int buttonH = design.navButtonHeight;

        const auto clearTooltip = [](QWidget *w) {
            if (!w) {
                return;
            }
            w->setProperty("i18nTooltipKey", QVariant());
            w->setToolTip(QString());
        };

        if (expanded) {
            m_settingsButton->setFixedSize(expandedW, buttonH);
            m_settingsButton->setText(QStringLiteral("  ") + i18n("nav.settings"));
            if (m_homeButton) {
                m_homeButton->setFixedSize(expandedW, buttonH);
                m_homeButton->setText(QStringLiteral("  ") + i18n("nav.home"));
            }
            if (m_aiReportButton) {
                m_aiReportButton->setFixedSize(expandedW, buttonH);
                m_aiReportButton->setText(QStringLiteral("  ") + i18n("nav.report"));
            }
            for (QPushButton *button : {m_settingsButton, m_homeButton, m_aiReportButton}) {
                if (!button) {
                    continue;
                }
                button->setProperty("sidebarExpanded", true);
                ThemeManager::instance().repolish(button);
            }
            clearTooltip(m_homeButton);
            clearTooltip(m_aiReportButton);
            clearTooltip(m_sidebarToggleButton);
            clearTooltip(m_settingsButton);
        } else {
            m_settingsButton->setFixedSize(collapsedW, buttonH);
            m_settingsButton->setText(QString());
            for (QPushButton *button : {m_settingsButton, m_homeButton, m_aiReportButton}) {
                if (!button) {
                    continue;
                }
                button->setProperty("sidebarExpanded", false);
                ThemeManager::instance().repolish(button);
            }
            if (m_homeButton) {
                m_homeButton->setFixedSize(collapsedW, buttonH);
                m_homeButton->setText(QString());
                i18nSetToolTip(m_homeButton, "tooltip.home");
            }
            if (m_aiReportButton) {
                m_aiReportButton->setFixedSize(collapsedW, buttonH);
                m_aiReportButton->setText(QString());
                i18nSetToolTip(m_aiReportButton, "tooltip.report");
            }
            i18nSetToolTip(m_sidebarToggleButton, "tooltip.expand");
            i18nSetToolTip(m_settingsButton, "tooltip.settings");
        }
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
    animMax->start(QAbstractAnimation::DeleteWhenStopped);
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
                                   i18n("tooltip.splitter"),
                                   handle);
            }
        } else if (event->type() == QEvent::Leave) {
            QToolTip::hideText();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
