#include "apptheme.h"

#include <QSettings>

// ═══════════════════════════════════════════════════════════════════
//  Accent Color 定义
// ═══════════════════════════════════════════════════════════════════

static QColor rawAccentColor(AccentColor accent)
{
    switch (accent) {
    case AccentColor::Purple:
        return QColor(124, 92, 255);
    case AccentColor::Blue:
        return QColor(0, 120, 212);
    case AccentColor::Orange:
        return QColor(255, 140, 0);
    }
    return QColor(124, 92, 255);
}

static QColor rawAccentHoverColor(AccentColor accent)
{
    switch (accent) {
    case AccentColor::Purple:
        return QColor(140, 115, 255);
    case AccentColor::Blue:
        return QColor(30, 145, 232);
    case AccentColor::Orange:
        return QColor(255, 160, 40);
    }
    return QColor(140, 115, 255);
}

QString accentColorName(AccentColor accent)
{
    switch (accent) {
    case AccentColor::Purple:
        return QStringLiteral("紫色");
    case AccentColor::Blue:
        return QStringLiteral("蓝色");
    case AccentColor::Orange:
        return QStringLiteral("橙色");
    }
    return QStringLiteral("紫色");
}

QColor accentColorValue(AccentColor accent, AppThemeKind kind)
{
    Q_UNUSED(kind);
    return rawAccentColor(accent);
}

QString accentColorStorageKey()
{
    return QStringLiteral("ui/accent");
}

AccentColor loadSavedAccentColor()
{
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const QString value = settings.value(accentColorStorageKey(), QStringLiteral("purple")).toString();
    if (value == QStringLiteral("blue")) return AccentColor::Blue;
    if (value == QStringLiteral("orange")) return AccentColor::Orange;
    return AccentColor::Purple;
}

void saveAccentColor(AccentColor accent)
{
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    QString value;
    switch (accent) {
    case AccentColor::Purple: value = QStringLiteral("purple"); break;
    case AccentColor::Blue:   value = QStringLiteral("blue");   break;
    case AccentColor::Orange: value = QStringLiteral("orange"); break;
    }
    settings.setValue(accentColorStorageKey(), value);
}

// ═══════════════════════════════════════════════════════════════════
//  Fluent Design 主题 – 完整 QSS 生成
// ═══════════════════════════════════════════════════════════════════

AppThemeColors appThemeColors(AppThemeKind kind, AccentColor accent)
{
    AppThemeColors theme;
    theme.kind = kind;
    theme.accent = accent;

    const QColor accentC = rawAccentColor(accent);
    const QColor accentHoverC = rawAccentHoverColor(accent);

    const QString accentStr = QStringLiteral("rgb(%1,%2,%3)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
    const QString accentHoverStr = QStringLiteral("rgb(%1,%2,%3)").arg(accentHoverC.red()).arg(accentHoverC.green()).arg(accentHoverC.blue());
    const QString accentAlpha25Str = QStringLiteral("rgba(%1,%2,%3,25)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
    const QString accentAlpha30Str = QStringLiteral("rgba(%1,%2,%3,30)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
    const QString accentAlpha35Str = QStringLiteral("rgba(%1,%2,%3,35)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
    const QString accentAlpha80Str = QStringLiteral("rgba(%1,%2,%3,80)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
    const QString accentAlpha100Str = QStringLiteral("rgba(%1,%2,%3,100)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());

    if (kind == AppThemeKind::Light) {
        // ── Light 浅色 ──────────────────────────────────────────
        theme.windowBackground = QStringLiteral("rgb(243, 243, 243)");
        theme.textPrimary = QStringLiteral("rgb(0, 0, 0)");
        theme.textSecondary = QStringLiteral("rgb(96, 96, 96)");
        theme.textMuted = QStringLiteral("rgb(128, 128, 128)");
        theme.textAccent = QStringLiteral("rgb(0, 103, 192)");
        theme.panelBackground = QStringLiteral("rgba(255,255,255,200)");
        theme.panelBorder = QStringLiteral("rgba(0,0,0,25)");
        theme.inputBackground = QStringLiteral("rgba(255,255,255,220)");
        theme.inputBorder = QStringLiteral("rgba(0,0,0,30)");
        theme.linkColor = QStringLiteral("rgb(0, 103, 192)");
        theme.linkHoverColor = QStringLiteral("rgb(0, 83, 162)");

        theme.sidebarToggleBackground = QStringLiteral("rgba(255,255,255,180)");
        theme.sidebarToggleBorder = QStringLiteral("rgba(0,0,0,20)");
        theme.sidebarToggleHover = QStringLiteral("rgba(255,255,255,230)");
        theme.sidebarToggleText = QStringLiteral("rgb(32, 32, 32)");

        theme.navButtonBackground = QStringLiteral("transparent");
        theme.navButtonBorder = QStringLiteral("transparent");
        theme.navButtonCheckedBackground = accentAlpha25Str;
        theme.navButtonCheckedBorder = accentAlpha80Str;
        theme.navButtonHoverBackground = QStringLiteral("rgba(0,0,0,8)");
        theme.navButtonText = QStringLiteral("rgb(0, 0, 0)");

        theme.periodButtonBackground = QStringLiteral("rgba(255,255,255,180)");
        theme.periodButtonBorder = QStringLiteral("rgba(0,0,0,15)");
        theme.periodButtonCheckedBackground = accentStr;
        theme.periodButtonText = QStringLiteral("rgb(0, 0, 0)");

        theme.splitterHandle = QStringLiteral("rgba(0,0,0,15)");
        theme.splitterHandleHover = QStringLiteral("rgba(0,0,0,30)");

        theme.scrollTrack = QStringLiteral("transparent");
        theme.scrollHandle = QStringLiteral("rgba(0,0,0,20)");
        theme.scrollHandleHover = QStringLiteral("rgba(0,0,0,35)");

        theme.listBackground = QStringLiteral("rgba(255,255,255,180)");
        theme.listItemBackground = QStringLiteral("rgba(255,255,255,220)");
        theme.listItemSelected = accentAlpha25Str;

        theme.primaryButtonBackground = accentStr;
        theme.primaryButtonHover = accentHoverStr;
        theme.primaryButtonDisabledBackground = QStringLiteral("rgba(120,120,130,80)");
        theme.primaryButtonDisabledText = QStringLiteral("rgba(150,150,160,120)");

        theme.secondaryButtonBackground = QStringLiteral("rgba(255,255,255,180)");
        theme.secondaryButtonBorder = QStringLiteral("rgba(0,0,0,15)");
        theme.secondaryButtonHover = QStringLiteral("rgba(255,255,255,230)");

        theme.chart.background = QColor(245, 245, 250);
        theme.chart.grid = QColor(215, 218, 228);
        theme.chart.axisText = QColor(128, 132, 142);
        theme.chart.bar = accentC;
        theme.chart.hoverLine = QColor(168, 172, 184);
        theme.chart.tooltipBackground = QColor(255, 255, 255, 240);
        theme.chart.tooltipText = QColor(52, 54, 60);
        theme.chart.placeholderIcon = QColor(168, 172, 184);
        return theme;
    }

    // ── Dark 深色 Fluent Design ─────────────────────────────────
    theme.windowBackground = QStringLiteral("#12121A");
    theme.textPrimary = QStringLiteral("rgba(255,255,255,220)");
    theme.textSecondary = QStringLiteral("rgba(200,200,210,180)");
    theme.textMuted = QStringLiteral("rgba(160,160,172,140)");
    theme.textAccent = QStringLiteral("rgb(140,180,255)");
    theme.panelBackground = QStringLiteral("rgba(35,35,54,180)");
    theme.panelBorder = QStringLiteral("rgba(255,255,255,20)");
    theme.inputBackground = QStringLiteral("rgba(45,45,65,160)");
    theme.inputBorder = QStringLiteral("rgba(255,255,255,18)");
    theme.linkColor = QStringLiteral("rgb(140,180,255)");
    theme.linkHoverColor = QStringLiteral("rgb(180,210,255)");

    theme.sidebarToggleBackground = QStringLiteral("rgba(35,35,54,160)");
    theme.sidebarToggleBorder = QStringLiteral("rgba(255,255,255,20)");
    theme.sidebarToggleHover = QStringLiteral("rgba(45,45,72,200)");
    theme.sidebarToggleText = QStringLiteral("rgba(255,255,255,200)");

    theme.navButtonBackground = QStringLiteral("transparent");
    theme.navButtonBorder = QStringLiteral("transparent");
    theme.navButtonCheckedBackground = accentAlpha35Str;
    theme.navButtonCheckedBorder = accentAlpha100Str;
    theme.navButtonHoverBackground = QStringLiteral("rgba(255,255,255,12)");
    theme.navButtonText = QStringLiteral("rgba(255,255,255,180)");

    theme.periodButtonBackground = QStringLiteral("rgba(45,45,72,140)");
    theme.periodButtonBorder = QStringLiteral("rgba(255,255,255,16)");
    theme.periodButtonCheckedBackground = accentStr;
    theme.periodButtonText = QStringLiteral("rgba(255,255,255,200)");

    theme.splitterHandle = QStringLiteral("rgba(255,255,255,15)");
    theme.splitterHandleHover = QStringLiteral("rgba(255,255,255,30)");

    theme.scrollTrack = QStringLiteral("transparent");
    theme.scrollHandle = QStringLiteral("rgba(255,255,255,20)");
    theme.scrollHandleHover = QStringLiteral("rgba(255,255,255,35)");

    theme.listBackground = QStringLiteral("rgba(35,35,54,180)");
    theme.listItemBackground = QStringLiteral("rgba(255,255,255,8)");
    theme.listItemSelected = accentAlpha30Str;

    theme.primaryButtonBackground = accentStr;
    theme.primaryButtonHover = accentHoverStr;
    theme.primaryButtonDisabledBackground = QStringLiteral("rgba(120,120,130,60)");
    theme.primaryButtonDisabledText = QStringLiteral("rgba(150,150,160,100)");

    theme.secondaryButtonBackground = QStringLiteral("rgba(45,45,72,140)");
    theme.secondaryButtonBorder = QStringLiteral("rgba(255,255,255,16)");
    theme.secondaryButtonHover = QStringLiteral("rgba(55,55,88,180)");

    theme.chart.background = QColor(24, 23, 38);
    theme.chart.grid = QColor(58, 56, 80);
    theme.chart.axisText = QColor(160, 158, 180);
    theme.chart.bar = accentC;
    theme.chart.hoverLine = QColor(110, 108, 135);
    theme.chart.tooltipBackground = QColor(40, 38, 58, 240);
    theme.chart.tooltipText = QColor(235, 235, 245);
    theme.chart.placeholderIcon = QColor(80, 78, 105);
    return theme;
}

QString themeStorageKey()
{
    return QStringLiteral("ui/theme");
}

AppThemeKind loadSavedThemeKind()
{
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    const QString value = settings.value(themeStorageKey(), QStringLiteral("dark")).toString();
    return value == QStringLiteral("light") ? AppThemeKind::Light : AppThemeKind::Dark;
}

void saveThemeKind(AppThemeKind kind)
{
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    settings.setValue(themeStorageKey(),
                       kind == AppThemeKind::Light ? QStringLiteral("light") : QStringLiteral("dark"));
}

// ═══════════════════════════════════════════════════════════════════
//  全局 + 各组件 QSS
// ═══════════════════════════════════════════════════════════════════

QString windowStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QMainWindow { background-color: %1; }"
               "QWidget#centralWidget { background-color: %1; }"
               "QWidget { color: %2; background: transparent; }"
               "QLabel { color: %2; background: transparent; }")
        .arg(theme.windowBackground, theme.textPrimary);
}

QString scrollAreaStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QScrollArea { border: none; background: transparent; }"
               "QScrollBar:vertical { background: transparent; width: 8px; margin: 0; border: none; }"
               "QScrollBar::handle:vertical { background: %1; border-radius: 4px; min-height: 32px; }"
               "QScrollBar::handle:vertical:hover { background: %2; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; border: none; background: transparent; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
        .arg(theme.scrollHandle, theme.scrollHandleHover);
}

QString settingsCardStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QFrame#settingsCard {"
               "  background-color: %1;"
               "  border: 1px solid %2;"
               "  border-radius: 16px;"
               "}"
               "QLabel { color: %3; font-size: 14px; background: transparent; }")
        .arg(theme.panelBackground, theme.panelBorder, theme.textPrimary);
}

QString comboBoxStyleSheet(const AppThemeColors &theme, bool useAccentHighlight)
{
    const QColor accentC = accentColorValue(theme.accent, theme.kind);
    const QString hoverStr = QStringLiteral("rgba(%1,%2,%3,100)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
    const QString selStr = QStringLiteral("rgba(%1,%2,%3,40)").arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());

    if (useAccentHighlight) {
        return QStringLiteral(
                   "QComboBox {"
                   "  color: %1; background-color: %2; border: 1px solid %3;"
                   "  border-radius: 8px; padding: 6px 12px;"
                   "}"
                   "QComboBox:hover { border-color: %4; }"
                   "QComboBox QAbstractItemView {"
                   "  color: %1; background-color: %5; border: 1px solid %3;"
                   "  border-radius: 8px; selection-background-color: %6;"
                   "}")
            .arg(theme.textPrimary, theme.inputBackground, theme.inputBorder,
                 hoverStr, theme.panelBackground, selStr);
    }
    return QStringLiteral(
               "QComboBox {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 8px; padding: 6px 12px;"
               "}"
               "QComboBox QAbstractItemView {"
               "  color: %1; background-color: %5; border: 1px solid %3;"
               "  border-radius: 8px; selection-background-color: %6;"
               "}")
        .arg(theme.textPrimary, theme.inputBackground, theme.inputBorder,
             theme.panelBackground, selStr);
}

QString lineEditStyleSheet(const AppThemeColors &theme)
{
    const QColor accentC = accentColorValue(theme.accent, theme.kind);
    return QStringLiteral(
               "QLineEdit {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 8px; padding: 8px 12px;"
               "}"
               "QLineEdit:focus { border-color: rgba(%4,%5,%6,150); }")
              .arg(theme.textPrimary, theme.inputBackground, theme.inputBorder)
              .arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
}

QString textEditStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QTextEdit {"
               "  background-color: %1; color: %2; border: 1px solid %3;"
               "  border-radius: 12px; padding: 12px; font-size: 13px;"
               "}")
        .arg(theme.inputBackground, theme.textSecondary, theme.panelBorder);
}

QString navButtonStyleSheet(const AppThemeColors &theme, bool expanded)
{
    const QString align = expanded ? QStringLiteral("left") : QStringLiteral("center");
    const QString padding = expanded ? QStringLiteral("padding-left: 14px;") : QString();
    return QStringLiteral(
               "QPushButton {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 10px; text-align: %4; %5 font-size: 14px;"
               "  padding-top: 8px; padding-bottom: 8px;"
               "}"
               "QPushButton:checked { background-color: %6; border-color: %7; }"
               "QPushButton:hover { background-color: %8; }")
        .arg(theme.navButtonText,
             theme.navButtonBackground,
             theme.navButtonBorder,
             align,
             padding,
             theme.navButtonCheckedBackground,
             theme.navButtonCheckedBorder,
             theme.navButtonHoverBackground);
}

QString sidebarToggleStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QPushButton {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 20px; font-size: 18px;"
               "}"
               "QPushButton:hover { background-color: %4; }")
        .arg(theme.sidebarToggleText,
             theme.sidebarToggleBackground,
             theme.sidebarToggleBorder,
             theme.sidebarToggleHover);
}

QString periodButtonStyleSheet(const AppThemeColors &theme)
{
    const QColor accentC = accentColorValue(theme.accent, theme.kind);
    return QStringLiteral(
               "QPushButton {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 10px; padding: 8px 24px; font-size: 13px; font-weight: 500;"
               "}"
               "QPushButton:checked { background-color: %4; border-color: %4; color: white; }"
               "QPushButton:hover:!checked { background-color: rgba(%5,%6,%7,30); }")
        .arg(theme.periodButtonText,
             theme.periodButtonBackground,
             theme.periodButtonBorder,
             theme.periodButtonCheckedBackground)
        .arg(accentC.red()).arg(accentC.green()).arg(accentC.blue());
}

QString chartPanelStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QFrame#chartCard {"
               "  background-color: %1;"
               "  border: 1px solid %2;"
               "  border-radius: 18px;"
               "}")
        .arg(theme.panelBackground, theme.panelBorder);
}

QString splitterStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QSplitter::handle:vertical {"
               "  background-color: %1; margin: 4px 24px; border-radius: 2px;"
               "}"
               "QSplitter::handle:vertical:hover { background-color: %2; }")
        .arg(theme.splitterHandle, theme.splitterHandleHover);
}

QString listWidgetStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QListWidget#appStatsList {"
               "  background-color: %1;"
               "  border: 1px solid %2;"
               "  border-radius: 18px;"
               "  color: %3; padding: 8px;"
               "}"
               "QListWidget::item {"
               "  background-color: %4; border-radius: 10px; margin: 2px 0; padding: 8px 12px;"
               "}"
               "QListWidget::item:selected { background-color: %5; }"
               "QScrollBar:vertical {"
               "  background: transparent; width: 6px; margin: 0; border: none;"
               "}"
               "QScrollBar::handle:vertical {"
               "  background: %6; border-radius: 3px; min-height: 36px;"
               "}"
               "QScrollBar::handle:vertical:hover { background: %7; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
               "  height: 0; border: none; background: transparent;"
               "}"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
        .arg(theme.listBackground,
             theme.panelBorder,
             theme.textPrimary,
             theme.listItemBackground,
             theme.listItemSelected,
             theme.scrollHandle,
             theme.scrollHandleHover);
}

QString primaryButtonStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QPushButton {"
               "  color: white; background-color: %1; border: none;"
               "  border-radius: 8px; padding: 8px 20px; font-size: 14px; font-weight: 500;"
               "}"
               "QPushButton:hover { background-color: %2; }"
               "QPushButton:disabled { background-color: %3; color: %4; }")
        .arg(theme.primaryButtonBackground,
             theme.primaryButtonHover,
             theme.primaryButtonDisabledBackground,
             theme.primaryButtonDisabledText);
}

QString secondaryButtonStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QPushButton {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 8px; padding: 8px 20px; font-size: 13px;"
               "}"
               "QPushButton:hover { background-color: %4; }")
        .arg(theme.textPrimary,
             theme.secondaryButtonBackground,
             theme.secondaryButtonBorder,
             theme.secondaryButtonHover);
}

QString linkButtonStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QPushButton { color: %1; background: transparent; border: none; text-align: left; padding: 0; }"
               "QPushButton:hover { color: %2; }")
              .arg(theme.linkColor, theme.linkHoverColor);
}

QString aiReportPageStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QLabel#aiReportSubtitle { color: %1; font-size: 13px; margin-bottom: 4px; }"
               "QLabel#aiReportStatus { color: %1; }"
               "QFrame#aiReportButtonRow {"
               "  background-color: %2;"
               "  border: 1px solid %3;"
               "  border-radius: 16px;"
               "}")
        .arg(theme.textMuted, theme.panelBackground, theme.panelBorder);
}

QString aiReportBlockStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QFrame {"
               "  background-color: %1;"
               "  border: 1px solid %2;"
               "  border-radius: 16px;"
               "}"
               "QLabel { color: %3; }"
               "QLabel#aiReportBlockTitle { font-size: 15px; font-weight: 600; color: %4; }"
               "QLabel#aiReportBlockContent { font-size: 14px; line-height: 1.6; padding: 4px 0; }"
               "QLabel#aiReportAutoTag { font-size: 12px; color: %5; }"
               "QPushButton#deleteReportButton {"
               "  color: %5; background-color: transparent; border: none;"
               "  font-size: 16px; padding: 0 6px; min-width: 24px; min-height: 24px;"
               "}"
               "QPushButton#deleteReportButton:hover { color: rgb(235,120,120); }")
        .arg(theme.panelBackground,
             theme.panelBorder,
             theme.textPrimary,
             theme.textAccent,
             theme.textMuted);
}
