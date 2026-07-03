#ifndef APPTHEME_H
#define APPTHEME_H

#include <QColor>
#include <QString>

enum class AppThemeKind {
    Dark,
    Light
};

enum class AccentColor {
    Purple,
    Blue,
    Orange
};

struct ChartThemeColors {
    QColor background;
    QColor grid;
    QColor axisText;
    QColor bar;
    QColor hoverLine;
    QColor tooltipBackground;
    QColor tooltipText;
    QColor placeholderIcon;
};

struct AppThemeColors {
    AppThemeKind kind = AppThemeKind::Dark;
    AccentColor accent = AccentColor::Purple;

    QString windowBackground;
    QString textPrimary;
    QString textSecondary;
    QString textMuted;
    QString textAccent;
    QString panelBackground;
    QString panelBorder;
    QString inputBackground;
    QString inputBorder;
    QString linkColor;
    QString linkHoverColor;

    QString sideBarBackground;
    QString sidebarToggleBackground;
    QString sidebarToggleBorder;
    QString sidebarToggleHover;
    QString sidebarToggleText;

    QString navButtonBackground;
    QString navButtonBorder;
    QString navButtonCheckedBackground;
    QString navButtonCheckedBorder;
    QString navButtonHoverBackground;
    QString navButtonText;

    QString periodButtonBackground;
    QString periodButtonBorder;
    QString periodButtonCheckedBackground;
    QString periodButtonText;

    QString splitterHandle;
    QString splitterHandleHover;

    QString scrollTrack;
    QString scrollHandle;
    QString scrollHandleHover;

    QString listBackground;
    QString listItemBackground;
    QString listItemSelected;

    QString primaryButtonBackground;
    QString primaryButtonHover;
    QString primaryButtonDisabledBackground;
    QString primaryButtonDisabledText;

    QString secondaryButtonBackground;
    QString secondaryButtonBorder;
    QString secondaryButtonHover;

    ChartThemeColors chart;
};
AppThemeColors appThemeColors(AppThemeKind kind, AccentColor accent = AccentColor::Purple);
QString themeStorageKey();
AppThemeKind loadSavedThemeKind();
void saveThemeKind(AppThemeKind kind);
QString accentColorStorageKey();
AccentColor loadSavedAccentColor();
void saveAccentColor(AccentColor accent);
QString accentColorName(AccentColor accent);
QColor accentColorValue(AccentColor accent, AppThemeKind kind);

QString windowStyleSheet(const AppThemeColors &theme);
QString scrollAreaStyleSheet(const AppThemeColors &theme);
QString settingsCardStyleSheet(const AppThemeColors &theme);
QString comboBoxStyleSheet(const AppThemeColors &theme, bool useAccentHighlight = true);
QString lineEditStyleSheet(const AppThemeColors &theme);
QString textEditStyleSheet(const AppThemeColors &theme);
QString navButtonStyleSheet(const AppThemeColors &theme, bool expanded);
QString sidebarToggleStyleSheet(const AppThemeColors &theme);
QString periodButtonStyleSheet(const AppThemeColors &theme);
QString chartPanelStyleSheet(const AppThemeColors &theme);
QString splitterStyleSheet(const AppThemeColors &theme);
QString listWidgetStyleSheet(const AppThemeColors &theme);
QString primaryButtonStyleSheet(const AppThemeColors &theme);
QString secondaryButtonStyleSheet(const AppThemeColors &theme);
QString linkButtonStyleSheet(const AppThemeColors &theme);
QString aiReportPageStyleSheet(const AppThemeColors &theme);
QString aiReportBlockStyleSheet(const AppThemeColors &theme);

#endif // APPTHEME_H

