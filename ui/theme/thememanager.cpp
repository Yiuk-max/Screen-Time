#include "thememanager.h"

#include "themestyles.h"

#include <QApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QWidget>

namespace {

ThemePalette makePalette(bool dark,
                         const QColor &background,
                         const QColor &sidebar,
                         const QColor &surface,
                         const QColor &elevatedSurface,
                         const QColor &textPrimary,
                         const QColor &textSecondary,
                         const QColor &textMuted,
                         const QColor &border,
                         const QColor &divider,
                         const AccentPalette &accent,
                         const QColor &success,
                         const QColor &warning,
                         const QColor &error,
                         const QColor &info,
                         const QColor &link,
                         const QColor &linkHover)
{
    ThemePalette p;
    p.dark = dark;
    p.background = background;
    p.sidebar = sidebar;
    p.surface = surface;
    p.elevatedSurface = elevatedSurface;
    p.textPrimary = textPrimary;
    p.textSecondary = textSecondary;
    p.textMuted = textMuted;
    p.border = border;
    p.divider = divider;
    p.accent = accent;
    p.success = success;
    p.warning = warning;
    p.error = error;
    p.info = info;
    p.link = link;
    p.linkHover = linkHover;
    return p;
}

// 由单个基色推导完整强调色板（hover / pressed / 文字色）。
AccentPalette accentFromBase(const QColor &base)
{
    AccentPalette p;
    p.accent = base;
    p.accentHover = blendColor(base, QColor(255, 255, 255), 48);
    p.accentPressed = blendColor(base, QColor(0, 0, 0), 48);
    p.accentText = isLightColor(base) ? QColor(26, 26, 26) : QColor(255, 255, 255);
    p.gradientEnd = p.accentHover;
    return p;
}

// 中国传统色「纸白 + 墨色」主题：浅色纸底 + 主色强调。
ThemePalette makePaperTheme(const QColor &paper, const QColor &ink)
{
    ThemePalette p;
    p.dark = false;
    p.background = paper;
    p.sidebar = blendColor(paper, ink, 24);
    p.surface = QColor(255, 255, 255);
    p.elevatedSurface = QColor(255, 255, 255);

    const QColor textBase = blendColor(ink, QColor(24, 24, 24), 170);
    p.textPrimary = textBase;
    p.textSecondary = blendColor(textBase, paper, 88);
    p.textMuted = blendColor(textBase, paper, 150);

    p.border = blendColor(paper, ink, 36);
    p.divider = blendColor(paper, ink, 20);

    p.accent = accentFromBase(ink);
    p.link = ink;
    p.linkHover = blendColor(ink, QColor(255, 255, 255), 40);

    p.success = QColor(QStringLiteral("#2E7D46"));
    p.warning = QColor(QStringLiteral("#B26A00"));
    p.error = QColor(QStringLiteral("#C0392B"));
    p.info = QColor(QStringLiteral("#1565C0"));
    return p;
}

QVector<ThemeDefinition> builtinThemes()
{
    QVector<ThemeDefinition> themes;

    // ── 跟随系统 ──────────────────────────────────────────────────
    {
        ThemeDefinition def;
        def.id = QStringLiteral("system");
        def.name = QStringLiteral("跟随系统");
        def.description = QStringLiteral("根据系统深色 / 浅色设置自动切换");
        def.followsSystem = true;
        def.palette.dark = false; // 占位，实际由 resolve() 决定
        themes.append(def);
    }

    // ── 浅色 ──────────────────────────────────────────────────────
    {
        ThemeDefinition def;
        def.id = QStringLiteral("light");
        def.name = QStringLiteral("浅色");
        def.description = QStringLiteral("中性灰白，适合日间使用");
        def.palette = makePalette(
            /*dark*/ false,
            QColor(QStringLiteral("#F5F6F8")), QColor(QStringLiteral("#ECEEF2")),
            QColor(QStringLiteral("#FFFFFF")), QColor(QStringLiteral("#FFFFFF")),
            QColor(QStringLiteral("#1A1D23")), QColor(QStringLiteral("#5A6069")),
            QColor(QStringLiteral("#8B919B")), QColor(QStringLiteral("#E1E4E9")),
            QColor(QStringLiteral("#EDEFF2")),
            ThemeManager::accentPalette(AccentColor::Blue),
            QColor(QStringLiteral("#16A34A")), QColor(QStringLiteral("#D97706")),
            QColor(QStringLiteral("#DC2626")), QColor(QStringLiteral("#0284C7")),
            QColor(QStringLiteral("#2563EB")), QColor(QStringLiteral("#1D4ED8")));
        themes.append(def);
    }

    // ── 深色 ──────────────────────────────────────────────────────
    {
        ThemeDefinition def;
        def.id = QStringLiteral("dark");
        def.name = QStringLiteral("深色");
        def.description = QStringLiteral("低亮度中性色，适合夜间使用");
        def.palette = makePalette(
            /*dark*/ true,
            QColor(QStringLiteral("#16181D")), QColor(QStringLiteral("#1B1E24")),
            QColor(QStringLiteral("#1F232A")), QColor(QStringLiteral("#262B33")),
            QColor(QStringLiteral("#E7E9EE")), QColor(QStringLiteral("#A2A8B4")),
            QColor(QStringLiteral("#6E7480")), QColor(QStringLiteral("#2C313A")),
            QColor(QStringLiteral("#262B33")),
            ThemeManager::accentPalette(AccentColor::Purple),
            QColor(QStringLiteral("#34D399")), QColor(QStringLiteral("#FBBF24")),
            QColor(QStringLiteral("#F87171")), QColor(QStringLiteral("#38BDF8")),
            QColor(QStringLiteral("#8FB0FF")), QColor(QStringLiteral("#B3C8FF")));
        themes.append(def);
    }

    // ── 中国传统色：纸白底色 + 主色 ───────────────────────────
    struct PaperTheme {
        const char *id;
        QString name;
        QString description;
        QString paper;
        QString ink;
    };
    const PaperTheme paperThemes[] = {
        {"iris",     QStringLiteral("鸢尾蓝"), QStringLiteral("宣纸白 · 鸢尾蓝"), QStringLiteral("#F9F2E0"), QStringLiteral("#1660AB")},
        {"amaranth", QStringLiteral("苋菜红"), QStringLiteral("石蕊红 · 苋菜红"), QStringLiteral("#F7ECE7"), QStringLiteral("#A61B29")},
        {"mushroom", QStringLiteral("蕈紫"),   QStringLiteral("豆汁黄 · 蕈紫"),   QStringLiteral("#FAFBE6"), QStringLiteral("#A48CE6")},
        {"scallion", QStringLiteral("葱油绿"), QStringLiteral("海天蓝 · 葱油绿"), QStringLiteral("#C6E6E8"), QStringLiteral("#373834")},
        {"alum",     QStringLiteral("青矾绿"), QStringLiteral("芡食白 · 青矾绿"), QStringLiteral("#F5F4F7"), QStringLiteral("#2C9678")},
        {"plum",     QStringLiteral("紫幽兰"), QStringLiteral("烟雨白 · 紫幽兰"), QStringLiteral("#E3E3E5"), QStringLiteral("#707899")},
    };
    for (const PaperTheme &entry : paperThemes) {
        ThemeDefinition def;
        def.id = QString::fromUtf8(entry.id);
        def.name = entry.name;
        def.description = entry.description;
        def.palette = makePaperTheme(QColor(entry.paper), QColor(entry.ink));
        themes.append(def);
    }

    return themes;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════
//  Accent palettes
// ═══════════════════════════════════════════════════════════════════
AccentPalette ThemeManager::accentPalette(AccentColor accent)
{
    AccentPalette p;
    switch (accent) {
    case AccentColor::Purple:
        p.accent = QColor(QStringLiteral("#8B5CF6"));
        p.accentHover = QColor(QStringLiteral("#A78BFA"));
        p.accentPressed = QColor(QStringLiteral("#7C3AED"));
        p.accentText = QColor(QStringLiteral("#FFFFFF"));
        p.gradientEnd = QColor(QStringLiteral("#3B82F6"));
        break;
    case AccentColor::Blue:
        p.accent = QColor(QStringLiteral("#3B82F6"));
        p.accentHover = QColor(QStringLiteral("#60A5FA"));
        p.accentPressed = QColor(QStringLiteral("#2563EB"));
        p.accentText = QColor(QStringLiteral("#FFFFFF"));
        p.gradientEnd = QColor(QStringLiteral("#22D3EE"));
        break;
    case AccentColor::Orange:
        p.accent = QColor(QStringLiteral("#F59E0B"));
        p.accentHover = QColor(QStringLiteral("#FBBF24"));
        p.accentPressed = QColor(QStringLiteral("#D97706"));
        p.accentText = QColor(QStringLiteral("#451A03"));
        p.gradientEnd = QColor(QStringLiteral("#EF4444"));
        break;
    case AccentColor::Theme:
        // 调用方（buildTheme）会忽略覆盖；这里给出一个中性兜底。
        p.accent = QColor(QStringLiteral("#3B82F6"));
        p.accentHover = QColor(QStringLiteral("#60A5FA"));
        p.accentPressed = QColor(QStringLiteral("#2563EB"));
        p.accentText = QColor(QStringLiteral("#FFFFFF"));
        p.gradientEnd = QColor(QStringLiteral("#22D3EE"));
        break;
    }
    return p;
}

QString ThemeManager::accentName(AccentColor accent)
{
    switch (accent) {
    case AccentColor::Theme:  return QStringLiteral("跟随主题");
    case AccentColor::Purple: return QStringLiteral("紫色");
    case AccentColor::Blue:   return QStringLiteral("蓝色");
    case AccentColor::Orange: return QStringLiteral("橙色");
    }
    return QStringLiteral("跟随主题");
}

// ═══════════════════════════════════════════════════════════════════
//  Singleton
// ═══════════════════════════════════════════════════════════════════
ThemeManager &ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager()
{
    for (const ThemeDefinition &definition : builtinThemes()) {
        registerTheme(definition);
    }

    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));

    m_themeId = settings.value(QStringLiteral("ui/theme"), QStringLiteral("system")).toString();
    // 迁移旧版本取值；旧的「极夜极光」土味主题已被移除，回退到深色。
    if (m_themeId == QStringLiteral("default")) {
        m_themeId = QStringLiteral("system");
    } else if (m_themeId == QStringLiteral("aurora") || m_themeId == QStringLiteral("transparent")) {
        m_themeId = QStringLiteral("dark");
    }

    const QString accentValue =
        settings.value(QStringLiteral("ui/accent"), QStringLiteral("theme")).toString();
    if (accentValue == QStringLiteral("purple")) {
        m_accent = AccentColor::Purple;
    } else if (accentValue == QStringLiteral("blue")) {
        m_accent = AccentColor::Blue;
    } else if (accentValue == QStringLiteral("orange")) {
        m_accent = AccentColor::Orange;
    } else {
        m_accent = AccentColor::Theme;
    }

    bool found = false;
    for (const ThemeDefinition &definition : m_definitions) {
        if (definition.id == m_themeId) {
            found = true;
            break;
        }
    }
    if (!found) {
        m_themeId = QStringLiteral("system");
    }

    resolve();
}

// ═══════════════════════════════════════════════════════════════════
//  Registration / query
// ═══════════════════════════════════════════════════════════════════
void ThemeManager::registerTheme(const ThemeDefinition &definition)
{
    for (int i = 0; i < m_definitions.size(); ++i) {
        if (m_definitions[i].id == definition.id) {
            m_definitions[i] = definition;
            return;
        }
    }
    m_definitions.append(definition);
}

ThemeOption ThemeManager::optionFor(const ThemeDefinition &definition) const
{
    ThemeOption option;
    option.id = definition.id;
    option.name = definition.name;
    option.description = definition.description;
    option.followsSystem = definition.followsSystem;
    return option;
}

QVector<ThemeOption> ThemeManager::themeOptions() const
{
    QVector<ThemeOption> options;
    options.reserve(m_definitions.size());
    for (const ThemeDefinition &definition : m_definitions) {
        options.append(optionFor(definition));
    }
    return options;
}

QVector<AccentOption> ThemeManager::accentOptions() const
{
    return {
        {AccentColor::Theme,  accentName(AccentColor::Theme)},
        {AccentColor::Purple, accentName(AccentColor::Purple)},
        {AccentColor::Blue,   accentName(AccentColor::Blue)},
        {AccentColor::Orange, accentName(AccentColor::Orange)},
    };
}

// ═══════════════════════════════════════════════════════════════════
//  Switching
// ═══════════════════════════════════════════════════════════════════
bool ThemeManager::setTheme(const QString &id)
{
    if (id == m_themeId) {
        return false;
    }

    bool found = false;
    for (const ThemeDefinition &definition : m_definitions) {
        if (definition.id == id) {
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }

    m_themeId = id;
    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    settings.setValue(QStringLiteral("ui/theme"), id);

    resolve();
    applyCurrent();
    emit themeChanged(m_theme);
    return true;
}

bool ThemeManager::setAccent(AccentColor accent)
{
    if (accent == m_accent) {
        return false;
    }
    m_accent = accent;

    QSettings settings(QStringLiteral("ScreenTime"), QStringLiteral("ScreenTime"));
    QString value = QStringLiteral("theme");
    switch (accent) {
    case AccentColor::Purple: value = QStringLiteral("purple"); break;
    case AccentColor::Blue:   value = QStringLiteral("blue");   break;
    case AccentColor::Orange: value = QStringLiteral("orange"); break;
    case AccentColor::Theme:  value = QStringLiteral("theme");  break;
    }
    settings.setValue(QStringLiteral("ui/accent"), value);

    resolve();
    applyCurrent();
    emit themeChanged(m_theme);
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Application
// ═══════════════════════════════════════════════════════════════════
void ThemeManager::applyTo(QApplication *app)
{
    m_app = app;
    applyCurrent();

    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
            if (m_themeId == QStringLiteral("system")) {
                resolve();
                applyCurrent();
                emit themeChanged(m_theme);
            }
        });
    }
}

void ThemeManager::repolish(QWidget *widget) const
{
    if (!widget) {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

// ═══════════════════════════════════════════════════════════════════
//  Resolution
// ═══════════════════════════════════════════════════════════════════
void ThemeManager::resolve()
{
    const ThemeDefinition *definition = nullptr;
    for (const ThemeDefinition &candidate : m_definitions) {
        if (candidate.id == m_themeId) {
            definition = &candidate;
            break;
        }
    }
    if (!definition) {
        return;
    }

    if (definition->followsSystem) {
        const QString target = systemPrefersDark() ? QStringLiteral("dark") : QStringLiteral("light");
        for (const ThemeDefinition &candidate : m_definitions) {
            if (candidate.id == target) {
                definition = &candidate;
                break;
            }
        }
    }

    const AccentPalette palette = accentPalette(m_accent);
    const AccentPalette *override = (m_accent == AccentColor::Theme) ? nullptr : &palette;
    m_theme = buildTheme(*definition, override);
}

void ThemeManager::applyCurrent()
{
    if (!m_app) {
        return;
    }
    m_app->setPalette(ThemeStyles::palette(m_theme));
    m_app->setStyleSheet(ThemeStyles::globalStyleSheet(m_theme));
}

bool ThemeManager::systemPrefersDark()
{
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark) {
        return true;
    }
    if (scheme == Qt::ColorScheme::Light) {
        return false;
    }
#ifdef Q_OS_WIN
    QSettings personalize(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return personalize.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return true;
#endif
}
