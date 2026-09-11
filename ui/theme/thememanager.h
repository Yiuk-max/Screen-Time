#ifndef THEME_THEMEMANAGER_H
#define THEME_THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>

#include "theme.h"

class QApplication;
class QWidget;

struct ThemeOption {
    QString id;
    QString name;
    QString description;
    bool followsSystem = false;
};

struct AccentOption {
    AccentColor value = AccentColor::Theme;
    QString name;
};

// 全局主题管理器：
//   * 持有当前 Theme 与强调色覆盖
//   * 负责注册 / 切换主题，并通过 themeChanged 通知 UI 刷新
//   * 负责把当前主题应用到 QApplication（调色板 + 全局 QSS）
//   * 负责持久化
//
// UI 代码不直接持有颜色；需要重绘的自绘控件只需连接 themeChanged。
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager &instance();

    // 注册主题（新增主题只需调用它，UI 组合框会自动列出）。
    void registerTheme(const ThemeDefinition &definition);

    QVector<ThemeOption> themeOptions() const;
    QVector<AccentOption> accentOptions() const;

    const Theme &theme() const { return m_theme; }
    QString themeId() const { return m_themeId; }
    AccentColor accent() const { return m_accent; }

    // 返回是否发生变化（变化时才 emit themeChanged）。
    bool setTheme(const QString &id);
    bool setAccent(AccentColor accent);

    // 挂载到 QApplication 并立刻应用当前主题。
    void applyTo(QApplication *app);

    // 主题变化后重新应用样式（动态属性改变时使用）。
    void repolish(QWidget *widget) const;

    static bool systemPrefersDark();
    static AccentPalette accentPalette(AccentColor accent);
    static QString accentName(AccentColor accent);

signals:
    void themeChanged(const Theme &theme);

private:
    ThemeManager();
    Q_DISABLE_COPY(ThemeManager)

    void resolve();
    void applyCurrent();
    ThemeOption optionFor(const ThemeDefinition &definition) const;

    QVector<ThemeDefinition> m_definitions;
    Theme m_theme;
    QString m_themeId;
    AccentColor m_accent = AccentColor::Theme;
    QApplication *m_app = nullptr;
};

#endif // THEME_THEMEMANAGER_H
