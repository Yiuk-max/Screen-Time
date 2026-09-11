#ifndef THEME_THEME_H
#define THEME_THEME_H

#include <QColor>
#include <QString>

// ═══════════════════════════════════════════════════════════════════
//  Design Tokens
//
//  与颜色无关的视觉参数（圆角、边框宽度、间距、字号、透明度、动画
//  时长等）。UI 代码只允许引用这里的字段，不允许再写魔法数字/具体
//  颜色，这样换主题时无需改动任何界面代码。
// ═══════════════════════════════════════════════════════════════════
struct DesignTokens {
    // 圆角
    int radiusXs = 3;
    int radiusSm = 6;
    int radiusMd = 8;
    int radiusLg = 10;
    int radiusXl = 16;
    int radiusPill = 999;
    int radiusCircle = 20;

    // 边框 / 描边宽度
    int borderWidth = 1;
    int focusBorderWidth = 1;
    int accentBarWidth = 3;

    // 间距
    int spaceXs = 4;
    int spaceSm = 8;
    int spaceMd = 12;
    int spaceLg = 16;
    int spaceXl = 24;

    // 字号 / 字重
    int fontSizeSm = 12;
    int fontSizeMd = 14;
    int fontSizeLg = 16;
    int fontSizeXl = 20;
    int weightNormal = 400;
    int weightMedium = 500;
    int weightSemiBold = 600;

    // 控件尺寸
    int controlHeight = 34;
    int controlHeightLg = 44;
    int iconSize = 18;
    int scrollBarWidth = 10;
    int scrollBarThickness = 8;
    int splitterHandleWidth = 8;

    // 半透明度（0-255），派生颜色时使用
    int hoverAlpha = 24;
    int pressedAlpha = 38;
    int subtleAlpha = 12;
    int selectedAlpha = 44;
    int disabledAlpha = 96;
    int trackAlpha = 22;
    int gridAlpha = 22;
    int hoverLineAlpha = 48;
    int scrollAlpha = 40;
    int scrollHoverAlpha = 72;

    // 动画时长
    int animFastMs = 140;
    int animNormalMs = 200;

    // 组件尺寸
    int sidebarExpandedWidth = 190;
    int sidebarCollapsedWidth = 68;
    int navButtonHeight = 44;
    int navButtonWidthExpanded = 174;
    int navButtonWidthCollapsed = 44;
};

// ═══════════════════════════════════════════════════════════════════
//  强调色
// ═══════════════════════════════════════════════════════════════════
enum class AccentColor {
    Theme,  // 跟随主题默认强调色
    Purple,
    Blue,
    Orange
};

struct AccentPalette {
    QColor accent;
    QColor accentHover;
    QColor accentPressed;
    QColor accentText;     // 强调色之上的文字（无效则自动按亮度取）
    QColor gradientEnd;    // 渐变终点（图表柱、进度条）
};

// ═══════════════════════════════════════════════════════════════════
//  图表颜色 Token
// ═══════════════════════════════════════════════════════════════════
struct ChartTokens {
    QColor background;
    QColor grid;
    QColor axisText;
    QColor bar;
    QColor barEnd;
    QColor barTrack;
    QColor hoverLine;
    QColor tooltipBackground;
    QColor tooltipBorder;
    QColor tooltipText;
    QColor placeholderIcon;
};

// ═══════════════════════════════════════════════════════════════════
//  语义化颜色 Token（完全解析后的结果，UI 只读这一层）
// ═══════════════════════════════════════════════════════════════════
struct ColorTokens {
    // 背景层级
    QColor background;
    QColor sidebar;
    QColor surface;
    QColor elevatedSurface;
    QColor surfaceHover;
    QColor surfacePressed;
    QColor overlay;

    // 文字
    QColor textPrimary;
    QColor textSecondary;
    QColor textMuted;
    QColor textDisabled;
    QColor textOnAccent;
    QColor link;
    QColor linkHover;

    // 线条
    QColor border;
    QColor divider;
    QColor focus;

    // 强调
    QColor accent;
    QColor accentHover;
    QColor accentPressed;
    QColor accentText;
    QColor accentSoft;
    QColor accentSubtle;

    // 状态
    QColor success;
    QColor successSoft;
    QColor warning;
    QColor warningSoft;
    QColor error;
    QColor errorSoft;
    QColor info;
    QColor infoSoft;

    // 控件
    QColor inputBackground;
    QColor inputBorder;
    QColor disabledBackground;
    QColor scrollHandle;
    QColor scrollHandleHover;
    QColor splitterHandle;
    QColor splitterHandleHover;

    // 浮层
    QColor tooltipBackground;
    QColor tooltipBorder;
    QColor tooltipText;
    QColor placeholderIcon;

    // 图表
    ChartTokens chart;

    bool dark = true;
};

// ═══════════════════════════════════════════════════════════════════
//  Theme Palette
//
//  主题作者只需要填写这一套「原始」颜色 Token，其余派生 Token
//  （hover、soft、chart track、scroll 等）由 buildColorTokens 统一
//  计算，保证所有主题视觉一致。
// ═══════════════════════════════════════════════════════════════════
struct ThemePalette {
    bool dark = true;

    QColor background;
    QColor sidebar;
    QColor surface;
    QColor elevatedSurface;

    QColor textPrimary;
    QColor textSecondary;
    QColor textMuted;

    QColor border;
    QColor divider;

    AccentPalette accent;

    QColor success;
    QColor warning;
    QColor error;
    QColor info;

    QColor link;       // 可选，默认取 accent
    QColor linkHover;  // 可选

    QColor chartBar;     // 可选，默认取 accent
    QColor chartBarEnd;  // 可选，默认取 accent 渐变终点
};

// ═══════════════════════════════════════════════════════════════════
//  Theme
// ═══════════════════════════════════════════════════════════════════
struct Theme {
    QString id;
    QString name;
    QString description;
    bool dark = true;
    bool followsSystem = false;
    ColorTokens colors;
    DesignTokens design;
};

// 主题定义：注册到 ThemeManager 的原始描述。
struct ThemeDefinition {
    QString id;
    QString name;
    QString description;
    bool followsSystem = false;
    ThemePalette palette;
    DesignTokens design;
};

// ── 工具函数 ──────────────────────────────────────────────────────
// 按 alpha（0-255）在 base 与 tint 之间做线性混合，结果不透明。
QColor blendColor(const QColor &base, const QColor &tint, int alpha);

// 判断颜色是否偏亮（用于自动选择强调色上的文字颜色）。
bool isLightColor(const QColor &color);

// 由主题原始色板 + DesignTokens 解析出完整的语义化颜色 Token。
ColorTokens buildColorTokens(const ThemePalette &palette, const DesignTokens &design);

// 由定义 + 强调色覆盖解析出完整主题。
Theme buildTheme(const ThemeDefinition &definition,
                 const AccentPalette *accentOverride = nullptr);

#endif // THEME_THEME_H
