#include "apptheme.h"

#include <QSettings>

AppThemeColors appThemeColors(AppThemeKind kind)
{
    AppThemeColors theme;
    theme.kind = kind;

    if (kind == AppThemeKind::Light) {
        // 柔和浅灰，避免纯白刺眼
        theme.windowBackground = QStringLiteral("rgb(218, 220, 226)");
        theme.textPrimary = QStringLiteral("rgb(52, 54, 60)");
        theme.textSecondary = QStringLiteral("rgb(92, 96, 104)");
        theme.textMuted = QStringLiteral("rgb(118, 122, 132)");
        theme.textAccent = QStringLiteral("rgb(56, 98, 168)");
        theme.panelBackground = QStringLiteral("rgb(232, 234, 240)");
        theme.panelBorder = QStringLiteral("rgb(198, 202, 212)");
        theme.inputBackground = QStringLiteral("rgb(224, 226, 232)");
        theme.inputBorder = QStringLiteral("rgb(190, 194, 204)");
        theme.linkColor = QStringLiteral("rgb(56, 98, 168)");
        theme.linkHoverColor = QStringLiteral("rgb(76, 118, 188)");

        theme.sidebarToggleBackground = QStringLiteral("rgb(228, 230, 236)");
        theme.sidebarToggleBorder = QStringLiteral("rgb(190, 194, 204)");
        theme.sidebarToggleHover = QStringLiteral("rgb(220, 222, 228)");
        theme.sidebarToggleText = QStringLiteral("rgb(68, 72, 80)");

        theme.navButtonBackground = QStringLiteral("rgb(228, 230, 236)");
        theme.navButtonBorder = QStringLiteral("rgb(190, 194, 204)");
        theme.navButtonCheckedBackground = QStringLiteral("rgb(214, 224, 242)");
        theme.navButtonCheckedBorder = QStringLiteral("rgb(160, 180, 220)");
        theme.navButtonHoverBackground = QStringLiteral("rgb(222, 224, 230)");
        theme.navButtonText = QStringLiteral("rgb(52, 54, 60)");

        theme.periodButtonBackground = QStringLiteral("rgb(228, 230, 236)");
        theme.periodButtonBorder = QStringLiteral("rgb(190, 194, 204)");
        theme.periodButtonCheckedBackground = QStringLiteral("rgb(214, 224, 242)");
        theme.periodButtonText = QStringLiteral("rgb(52, 54, 60)");

        theme.splitterHandle = QStringLiteral("rgb(190, 194, 204)");
        theme.splitterHandleHover = QStringLiteral("rgb(168, 172, 184)");

        theme.scrollTrack = QStringLiteral("rgb(210, 212, 220)");
        theme.scrollHandle = QStringLiteral("rgb(178, 182, 192)");
        theme.scrollHandleHover = QStringLiteral("rgb(158, 162, 174)");

        theme.listBackground = QStringLiteral("rgb(232, 234, 240)");
        theme.listItemBackground = QStringLiteral("rgb(224, 226, 232)");
        theme.listItemSelected = QStringLiteral("rgb(214, 224, 242)");

        theme.primaryButtonBackground = QStringLiteral("rgb(70,130,180)");
        theme.primaryButtonHover = QStringLiteral("rgb(90,150,200)");
        theme.primaryButtonDisabledBackground = QStringLiteral("rgb(200, 204, 214)");
        theme.primaryButtonDisabledText = QStringLiteral("rgb(140, 144, 154)");

        theme.secondaryButtonBackground = QStringLiteral("rgb(224, 226, 232)");
        theme.secondaryButtonBorder = QStringLiteral("rgb(190, 194, 204)");
        theme.secondaryButtonHover = QStringLiteral("rgb(216, 218, 226)");

        theme.chart.background = QColor(228, 230, 236);
        theme.chart.grid = QColor(198, 202, 212);
        theme.chart.axisText = QColor(108, 112, 122);
        theme.chart.bar = QColor(45, 135, 255);
        theme.chart.hoverLine = QColor(158, 162, 174);
        theme.chart.tooltipBackground = QColor(236, 238, 244, 245);
        theme.chart.tooltipText = QColor(52, 54, 60);
        theme.chart.placeholderIcon = QColor(158, 162, 174);
        return theme;
    }

    theme.windowBackground = QStringLiteral("rgb(24,24,26)");
    theme.textPrimary = QStringLiteral("rgb(235,235,240)");
    theme.textSecondary = QStringLiteral("rgb(200,200,205)");
    theme.textMuted = QStringLiteral("rgb(150,150,155)");
    theme.textAccent = QStringLiteral("rgb(180,200,230)");
    theme.panelBackground = QStringLiteral("rgb(36,36,40)");
    theme.panelBorder = QStringLiteral("rgb(58,58,64)");
    theme.inputBackground = QStringLiteral("rgb(50,50,55)");
    theme.inputBorder = QStringLiteral("rgb(78,78,84)");
    theme.linkColor = QStringLiteral("rgb(157,199,255)");
    theme.linkHoverColor = QStringLiteral("rgb(187,217,255)");

    theme.sidebarToggleBackground = QStringLiteral("rgb(42,42,46)");
    theme.sidebarToggleBorder = QStringLiteral("rgb(58,58,64)");
    theme.sidebarToggleHover = QStringLiteral("rgb(52,52,58)");
    theme.sidebarToggleText = QStringLiteral("rgb(210,210,215)");

    theme.navButtonBackground = QStringLiteral("rgb(34,34,38)");
    theme.navButtonBorder = QStringLiteral("rgb(48,48,52)");
    theme.navButtonCheckedBackground = QStringLiteral("rgb(44,44,48)");
    theme.navButtonCheckedBorder = QStringLiteral("rgb(76,76,84)");
    theme.navButtonHoverBackground = QStringLiteral("rgb(42,42,46)");
    theme.navButtonText = QStringLiteral("rgb(220,220,225)");

    theme.periodButtonBackground = QStringLiteral("rgb(34,34,38)");
    theme.periodButtonBorder = QStringLiteral("rgb(48,48,52)");
    theme.periodButtonCheckedBackground = QStringLiteral("rgb(44,44,48)");
    theme.periodButtonText = QStringLiteral("rgb(220,220,225)");

    theme.splitterHandle = QStringLiteral("rgb(58,58,64)");
    theme.splitterHandleHover = QStringLiteral("rgb(88,88,96)");

    theme.scrollTrack = QStringLiteral("rgb(40,40,44)");
    theme.scrollHandle = QStringLiteral("rgb(80,80,85)");
    theme.scrollHandleHover = QStringLiteral("rgb(100,100,105)");

    theme.listBackground = QStringLiteral("rgb(36,36,40)");
    theme.listItemBackground = QStringLiteral("rgb(42,42,46)");
    theme.listItemSelected = QStringLiteral("rgb(52,52,58)");

    theme.primaryButtonBackground = QStringLiteral("rgb(70,130,180)");
    theme.primaryButtonHover = QStringLiteral("rgb(90,150,200)");
    theme.primaryButtonDisabledBackground = QStringLiteral("rgb(60,60,65)");
    theme.primaryButtonDisabledText = QStringLiteral("rgb(150,150,155)");

    theme.secondaryButtonBackground = QStringLiteral("rgb(50,50,55)");
    theme.secondaryButtonBorder = QStringLiteral("rgb(78,78,84)");
    theme.secondaryButtonHover = QStringLiteral("rgb(60,60,65)");

    theme.chart.background = QColor(28, 28, 30);
    theme.chart.grid = QColor(78, 78, 82);
    theme.chart.axisText = QColor(150, 150, 156);
    theme.chart.bar = QColor(45, 135, 255);
    theme.chart.hoverLine = QColor(110, 110, 115);
    theme.chart.tooltipBackground = QColor(56, 56, 60, 230);
    theme.chart.tooltipText = QColor(235, 235, 240);
    theme.chart.placeholderIcon = QColor(120, 120, 126);
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

QString windowStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QMainWindow { background-color: %1; }"
               "QWidget { color: %2; background-color: transparent; }"
               "QLabel { color: %2; }")
        .arg(theme.windowBackground, theme.textPrimary);
}

QString scrollAreaStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollBar:vertical { background: %1; width: 8px; }"
               "QScrollBar::handle:vertical { background: %2; border-radius: 4px; }"
               "QScrollBar::handle:vertical:hover { background: %3; }")
        .arg(theme.scrollTrack, theme.scrollHandle, theme.scrollHandleHover);
}

QString settingsCardStyleSheet(const AppThemeColors &theme)
{
    const QString frameBorder = theme.kind == AppThemeKind::Dark
        ? QStringLiteral("border: none;")
        : QStringLiteral("border: 1px solid %1;").arg(theme.panelBorder);
    return QStringLiteral(
               "QFrame { background-color: %1; border-radius: 10px; %2 }"
               "QLabel { color: %3; font-size: 14px; }")
        .arg(theme.panelBackground, frameBorder, theme.textPrimary);
}

QString comboBoxStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QComboBox {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 6px; padding: 4px 8px;"
               "}"
               "QComboBox QAbstractItemView {"
               "  color: %1; background-color: %4; border: 1px solid %3;"
               "}")
        .arg(theme.textPrimary, theme.inputBackground, theme.inputBorder, theme.panelBackground);
}

QString lineEditStyleSheet(const AppThemeColors &theme)
{
    return settingsCardStyleSheet(theme)
        + QStringLiteral(
              "QLineEdit {"
              "  color: %1; background-color: %2; border: 1px solid %3;"
              "  border-radius: 6px; padding: 6px 10px;"
              "}"
              "QLineEdit:focus { border-color: rgb(70,130,180); }")
              .arg(theme.textPrimary, theme.inputBackground, theme.inputBorder);
}

QString textEditStyleSheet(const AppThemeColors &theme)
{
    const QString bg = theme.kind == AppThemeKind::Dark
        ? QStringLiteral("rgb(30,30,34)")
        : theme.inputBackground;
    const QString frameBorder = theme.kind == AppThemeKind::Dark
        ? QStringLiteral("border: 1px solid %1;").arg(theme.panelBorder)
        : QStringLiteral("border: 1px solid %1;").arg(theme.inputBorder);
    return QStringLiteral(
               "QTextEdit {"
               "  background-color: %1; color: %2; %3"
               "  border-radius: 8px; padding: 8px; font-size: 13px;"
               "}")
        .arg(bg, theme.textSecondary, frameBorder);
}

QString navButtonStyleSheet(const AppThemeColors &theme, bool expanded)
{
    const QString align = expanded ? QStringLiteral("left") : QStringLiteral("center");
    const QString padding = expanded ? QStringLiteral("padding-left: 10px;") : QString();
    return QStringLiteral(
               "QPushButton {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 8px; text-align: %4; %5 font-size: 14px;"
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
    return QStringLiteral(
               "QPushButton {"
               "  color: %1; background-color: %2; border: 1px solid %3;"
               "  border-radius: 8px; padding: 6px 16px;"
               "}"
               "QPushButton:checked { background-color: %4; }"
               "QPushButton:hover { background-color: %5; }")
        .arg(theme.periodButtonText,
             theme.periodButtonBackground,
             theme.periodButtonBorder,
             theme.periodButtonCheckedBackground,
             theme.navButtonHoverBackground);
}

QString chartPanelStyleSheet(const AppThemeColors &theme)
{
    if (theme.kind == AppThemeKind::Dark) {
        return QStringLiteral("QFrame { background-color: %1; border-radius: 12px; border: none; }")
            .arg(theme.panelBackground);
    }
    return QStringLiteral("QFrame { background-color: %1; border-radius: 12px; border: 1px solid %2; }")
        .arg(theme.panelBackground, theme.panelBorder);
}

QString splitterStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QSplitter::handle:vertical {"
               "  background-color: %1; margin: 2px 10px; border-radius: 3px;"
               "}"
               "QSplitter::handle:vertical:hover { background-color: %2; }")
        .arg(theme.splitterHandle, theme.splitterHandleHover);
}

QString listWidgetStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QListWidget {"
               "  background-color: %1; border: 1px solid %2; border-radius: 10px;"
               "  color: %3; padding: 4px;"
               "}"
               "QListWidget::item {"
               "  background-color: %4; border-radius: 8px; margin: 2px 0; padding: 6px 8px;"
               "}"
               "QListWidget::item:selected { background-color: %5; }")
        .arg(theme.listBackground,
             theme.panelBorder,
             theme.textPrimary,
             theme.listItemBackground,
             theme.listItemSelected);
}

QString primaryButtonStyleSheet(const AppThemeColors &theme)
{
    return QStringLiteral(
               "QPushButton {"
               "  color: white; background-color: %1; border: none;"
               "  border-radius: 6px; padding: 8px 20px; font-size: 14px;"
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
               "  border-radius: 6px; padding: 6px 16px;"
               "}"
               "QPushButton:hover { background-color: %4; }")
        .arg(theme.textPrimary,
             theme.secondaryButtonBackground,
             theme.secondaryButtonBorder,
             theme.secondaryButtonHover);
}

QString linkButtonStyleSheet(const AppThemeColors &theme)
{
    return settingsCardStyleSheet(theme)
        + QStringLiteral(
              "QPushButton { color: %1; background: transparent; border: none; text-align: left; padding: 0; }"
              "QPushButton:hover { color: %2; }")
              .arg(theme.linkColor, theme.linkHoverColor);
}

QString aiReportPageStyleSheet(const AppThemeColors &theme)
{
    const QString frameBorder = theme.kind == AppThemeKind::Dark
        ? QStringLiteral("border: none;")
        : QStringLiteral("border: 1px solid %1;").arg(theme.panelBorder);
    return QStringLiteral(
               "QLabel#aiReportSubtitle { color: %1; font-size: 13px; margin-bottom: 4px; }"
               "QLabel#aiReportStatus { color: %1; }"
               "QFrame#aiReportButtonRow { background-color: %2; border-radius: 10px; %3 }")
        .arg(theme.textMuted, theme.panelBackground, frameBorder);
}

QString aiReportBlockStyleSheet(const AppThemeColors &theme)
{
    const QString frameBorder = theme.kind == AppThemeKind::Dark
        ? QStringLiteral("border: none;")
        : QStringLiteral("border: 1px solid %1;").arg(theme.panelBorder);
    return QStringLiteral(
               "QFrame { background-color: %1; border-radius: 10px; %2 }"
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
             frameBorder,
             theme.textPrimary,
             theme.textAccent,
             theme.textMuted);
}
