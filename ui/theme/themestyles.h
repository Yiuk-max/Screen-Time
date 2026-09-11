#ifndef THEME_THEMESTYLES_H
#define THEME_THEMESTYLES_H

#include <QIcon>
#include <QPalette>
#include <QSize>
#include <QString>

#include "theme.h"

// 由当前 Theme 生成全局 QSS / QPalette。所有具体颜色值都来自 Token，
// 这里不出现任何硬编码颜色。
namespace ThemeStyles {

QPalette palette(const Theme &theme);

QString globalStyleSheet(const Theme &theme);

// 把模板 SVG 中的 currentColor 替换为主题颜色后渲染为图标。
QIcon svgIcon(const QString &svgTemplate, const QColor &color, const QSize &size);

// 供 QSS 使用：带 alpha 时输出 rgba()，否则输出 #RRGGBB。
QString colorValue(const QColor &color);

} // namespace ThemeStyles

#endif // THEME_THEMESTYLES_H
