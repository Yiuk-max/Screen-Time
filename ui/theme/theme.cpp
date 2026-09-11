#include "theme.h"

#include <QtGlobal>
#include <cmath>

QColor blendColor(const QColor &base, const QColor &tint, int alpha)
{
    const qreal t = qBound(0, alpha, 255) / 255.0;
    return QColor(
        qRound(base.red() * (1.0 - t) + tint.red() * t),
        qRound(base.green() * (1.0 - t) + tint.green() * t),
        qRound(base.blue() * (1.0 - t) + tint.blue() * t));
}

bool isLightColor(const QColor &color)
{
    const qreal luminance =
        0.2126 * color.redF() + 0.7152 * color.greenF() + 0.0722 * color.blueF();
    return luminance > 0.6;
}

static QColor orFallback(const QColor &value, const QColor &fallback)
{
    return value.isValid() ? value : fallback;
}

ColorTokens buildColorTokens(const ThemePalette &p, const DesignTokens &d)
{
    ColorTokens c;
    c.dark = p.dark;

    // ── 背景层级 ────────────────────────────────────────────────
    c.background = p.background;
    c.sidebar = orFallback(p.sidebar, p.background);
    c.surface = p.surface;
    c.elevatedSurface = orFallback(p.elevatedSurface, p.surface);

    // hover / pressed 用主文字色做方向参考：深色主题变亮，浅色主题变暗。
    const QColor tint = p.textPrimary;
    c.surfaceHover = blendColor(c.surface, tint, d.hoverAlpha);
    c.surfacePressed = blendColor(c.surface, tint, d.pressedAlpha);
    c.overlay = blendColor(c.background, tint, d.subtleAlpha);

    // ── 文字 ────────────────────────────────────────────────────
    c.textPrimary = p.textPrimary;
    c.textSecondary = p.textSecondary;
    c.textMuted = p.textMuted;
    c.textDisabled = blendColor(p.textMuted, p.background, d.disabledAlpha);
    c.link = orFallback(p.link, p.accent.accent);
    c.linkHover = orFallback(p.linkHover, p.accent.accentHover);

    // ── 线条 ────────────────────────────────────────────────────
    c.border = p.border;
    c.divider = orFallback(p.divider, p.border);
    c.focus = p.accent.accent;

    // ── 强调色 ──────────────────────────────────────────────────
    c.accent = p.accent.accent;
    c.accentHover = p.accent.accentHover;
    c.accentPressed = p.accent.accentPressed;
    c.accentText = orFallback(
        p.accent.accentText,
        isLightColor(p.accent.accent) ? QColor(16, 17, 20) : QColor(255, 255, 255));
    c.textOnAccent = c.accentText;
    c.accentSoft = blendColor(c.elevatedSurface, c.accent, d.selectedAlpha);
    c.accentSubtle = blendColor(c.elevatedSurface, c.accent, d.selectedAlpha / 2);

    // ── 状态色 ──────────────────────────────────────────────────
    c.success = p.success;
    c.warning = p.warning;
    c.error = p.error;
    c.info = p.info;
    c.successSoft = blendColor(c.elevatedSurface, c.success, d.selectedAlpha);
    c.warningSoft = blendColor(c.elevatedSurface, c.warning, d.selectedAlpha);
    c.errorSoft = blendColor(c.elevatedSurface, c.error, d.selectedAlpha);
    c.infoSoft = blendColor(c.elevatedSurface, c.info, d.selectedAlpha);

    // ── 控件 ────────────────────────────────────────────────────
    c.inputBackground = c.elevatedSurface;
    c.inputBorder = c.border;
    c.disabledBackground = blendColor(c.surface, tint, d.subtleAlpha);
    c.scrollHandle = blendColor(c.surface, tint, d.scrollAlpha);
    c.scrollHandleHover = blendColor(c.surface, tint, d.scrollHoverAlpha);
    c.splitterHandle = c.divider;
    c.splitterHandleHover = blendColor(c.surface, tint, d.hoverAlpha);

    // ── 浮层 ────────────────────────────────────────────────────
    c.tooltipBackground = c.elevatedSurface;
    c.tooltipBorder = c.border;
    c.tooltipText = c.textPrimary;
    c.placeholderIcon = c.textMuted;

    // ── 图表 ────────────────────────────────────────────────────
    c.chart.background = QColor(0, 0, 0, 0);
    c.chart.grid = blendColor(c.surface, tint, d.gridAlpha);
    c.chart.axisText = c.textMuted;
    c.chart.bar = orFallback(p.chartBar, c.accent);
    c.chart.barEnd = orFallback(p.chartBarEnd,
                                p.accent.gradientEnd.isValid() ? p.accent.gradientEnd
                                                               : c.accentHover);
    c.chart.barTrack = blendColor(c.surface, tint, d.trackAlpha);
    c.chart.hoverLine = blendColor(c.surface, tint, d.hoverLineAlpha);
    c.chart.tooltipBackground = c.tooltipBackground;
    c.chart.tooltipBorder = c.tooltipBorder;
    c.chart.tooltipText = c.tooltipText;
    c.chart.placeholderIcon = c.placeholderIcon;

    return c;
}

Theme buildTheme(const ThemeDefinition &definition, const AccentPalette *accentOverride)
{
    Theme theme;
    theme.id = definition.id;
    theme.name = definition.name;
    theme.description = definition.description;
    theme.followsSystem = definition.followsSystem;
    theme.design = definition.design;

    ThemePalette palette = definition.palette;
    if (accentOverride && accentOverride->accent.isValid()) {
        palette.accent = *accentOverride;
    }
    theme.dark = palette.dark;
    theme.colors = buildColorTokens(palette, theme.design);
    return theme;
}
