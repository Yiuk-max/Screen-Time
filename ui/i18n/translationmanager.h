#ifndef I18N_TRANSLATIONMANAGER_H
#define I18N_TRANSLATIONMANAGER_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class QLabel;
class QAbstractButton;
class QWidget;

struct LanguageInfo {
    QString id;          // locale id, e.g. "de"
    QString nativeName;  // e.g. "Deutsch"
    QString englishName; // e.g. "German"
};

// 轻量翻译管理器：内置各语言的字符串表，支持运行时切换并通知界面刷新。
// 基准语言为简体中文，其余语言缺失的 key 自动回退到中文。
class TranslationManager : public QObject
{
    Q_OBJECT

public:
    static TranslationManager &instance();

    QVector<LanguageInfo> languages() const;
    QString languageId() const { return m_languageId; }

    // 返回是否发生变化（变化时才 emit languageChanged）。
    bool setLanguage(const QString &id);

    bool has(const QString &key) const;
    QString text(const QString &key) const;
    QStringList keys() const { return m_base.keys(); }
    // 诊断用：返回某语言缺失的 key（基准语言返回空）。
    QStringList missingKeys(const QString &languageId) const;

signals:
    void languageChanged();

private:
    TranslationManager();
    Q_DISABLE_COPY(TranslationManager)

    QString m_languageId;
    QHash<QString, QString> m_base;                       // key -> 简体中文
    QHash<QString, QHash<QString, QString>> m_tables;     // locale -> key -> 译文
};

// ── 便捷函数 ──────────────────────────────────────────────────────
inline QString i18n(const char *key)
{
    return TranslationManager::instance().text(QString::fromLatin1(key));
}

inline QString i18n(const QString &key)
{
    return TranslationManager::instance().text(key);
}

// 给控件打上 i18nKey / i18nTooltipKey 属性，并立即应用当前语言。
void i18nSetText(QLabel *label, const char *key);
void i18nSetText(QAbstractButton *button, const char *key);
void i18nSetToolTip(QWidget *widget, const char *key);

// 遍历子树，根据 i18nKey / i18nTooltipKey 属性重新设置文本（语言切换时调用）。
void i18nRetranslate(QWidget *root);

#endif // I18N_TRANSLATIONMANAGER_H
