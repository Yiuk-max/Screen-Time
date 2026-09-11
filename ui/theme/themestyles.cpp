#include "themestyles.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QSvgRenderer>

namespace {

// 把主题色的下拉箭头写入临时文件，供 QSS 的 url() 引用。
// 原生样式在深色背景下常常画不出箭头，用主题色自绘可保证可见。
QString comboArrowPath(const QColor &color)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    QDir().mkpath(dir);

    const QString fileName = QStringLiteral("screentime_combo_arrow_%1.svg")
                                 .arg(color.name(QColor::HexRgb).mid(1));
    const QString path = dir + QLatin1Char('/') + fileName;

    const QByteArray svg = QStringLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 12 12\">"
        "<path fill=\"%1\" d=\"M2.2 4.2 6 8l3.8-3.8z\"/></svg>")
        .arg(color.name(QColor::HexRgb)).toUtf8();

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(svg);
        file.close();
    }
    QString urlPath = path;
    urlPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return urlPath;
}

QString applyTokens(QString css, const QHash<QString, QString> &tokens)
{
    for (auto it = tokens.cbegin(); it != tokens.cend(); ++it) {
        css.replace(it.key(), it.value());
    }
    return css;
}

QHash<QString, QString> buildTokenMap(const Theme &theme)
{
    const ColorTokens &c = theme.colors;
    const DesignTokens &d = theme.design;

    QHash<QString, QString> t;
    const auto add = [&t](const QString &key, const QColor &color) {
        t.insert(key, ThemeStyles::colorValue(color));
    };
    const auto addInt = [&t](const QString &key, int value) {
        t.insert(key, QString::number(value));
    };

    // 颜色 Token
    add(QStringLiteral("%background%"), c.background);
    add(QStringLiteral("%sidebar%"), c.sidebar);
    add(QStringLiteral("%surface%"), c.surface);
    add(QStringLiteral("%elevatedSurface%"), c.elevatedSurface);
    add(QStringLiteral("%surfaceHover%"), c.surfaceHover);
    add(QStringLiteral("%surfacePressed%"), c.surfacePressed);
    add(QStringLiteral("%overlay%"), c.overlay);

    add(QStringLiteral("%textPrimary%"), c.textPrimary);
    add(QStringLiteral("%textSecondary%"), c.textSecondary);
    add(QStringLiteral("%textMuted%"), c.textMuted);
    add(QStringLiteral("%textDisabled%"), c.textDisabled);
    add(QStringLiteral("%textOnAccent%"), c.textOnAccent);
    add(QStringLiteral("%link%"), c.link);
    add(QStringLiteral("%linkHover%"), c.linkHover);

    add(QStringLiteral("%border%"), c.border);
    add(QStringLiteral("%divider%"), c.divider);
    add(QStringLiteral("%focus%"), c.focus);

    add(QStringLiteral("%accent%"), c.accent);
    add(QStringLiteral("%accentHover%"), c.accentHover);
    add(QStringLiteral("%accentPressed%"), c.accentPressed);
    add(QStringLiteral("%accentText%"), c.accentText);
    add(QStringLiteral("%accentSoft%"), c.accentSoft);
    add(QStringLiteral("%accentSubtle%"), c.accentSubtle);

    add(QStringLiteral("%success%"), c.success);
    add(QStringLiteral("%successSoft%"), c.successSoft);
    add(QStringLiteral("%warning%"), c.warning);
    add(QStringLiteral("%warningSoft%"), c.warningSoft);
    add(QStringLiteral("%error%"), c.error);
    add(QStringLiteral("%errorSoft%"), c.errorSoft);
    add(QStringLiteral("%info%"), c.info);
    add(QStringLiteral("%infoSoft%"), c.infoSoft);

    add(QStringLiteral("%inputBackground%"), c.inputBackground);
    add(QStringLiteral("%inputBorder%"), c.inputBorder);
    add(QStringLiteral("%disabledBackground%"), c.disabledBackground);
    add(QStringLiteral("%scrollHandle%"), c.scrollHandle);
    add(QStringLiteral("%scrollHandleHover%"), c.scrollHandleHover);
    add(QStringLiteral("%splitterHandle%"), c.splitterHandle);
    add(QStringLiteral("%splitterHandleHover%"), c.splitterHandleHover);

    add(QStringLiteral("%tooltipBackground%"), c.tooltipBackground);
    add(QStringLiteral("%tooltipBorder%"), c.tooltipBorder);
    add(QStringLiteral("%tooltipText%"), c.tooltipText);
    add(QStringLiteral("%placeholderIcon%"), c.placeholderIcon);

    // 主题色下拉箭头（写入临时文件，QSS 用 url() 引用）。
    t.insert(QStringLiteral("%comboArrow%"), comboArrowPath(c.textSecondary));

    // 设计 Token
    addInt(QStringLiteral("%radiusXs%"), d.radiusXs);
    addInt(QStringLiteral("%radiusSm%"), d.radiusSm);
    addInt(QStringLiteral("%radiusMd%"), d.radiusMd);
    addInt(QStringLiteral("%radiusLg%"), d.radiusLg);
    addInt(QStringLiteral("%radiusXl%"), d.radiusXl);
    addInt(QStringLiteral("%radiusPill%"), d.radiusPill);
    addInt(QStringLiteral("%radiusCircle%"), d.radiusCircle);
    addInt(QStringLiteral("%borderWidth%"), d.borderWidth);
    addInt(QStringLiteral("%accentBarWidth%"), d.accentBarWidth);
    addInt(QStringLiteral("%spaceXs%"), d.spaceXs);
    addInt(QStringLiteral("%spaceSm%"), d.spaceSm);
    addInt(QStringLiteral("%spaceMd%"), d.spaceMd);
    addInt(QStringLiteral("%spaceLg%"), d.spaceLg);
    addInt(QStringLiteral("%spaceXl%"), d.spaceXl);
    addInt(QStringLiteral("%fontSizeSm%"), d.fontSizeSm);
    addInt(QStringLiteral("%fontSizeMd%"), d.fontSizeMd);
    addInt(QStringLiteral("%fontSizeLg%"), d.fontSizeLg);
    addInt(QStringLiteral("%fontSizeXl%"), d.fontSizeXl);
    addInt(QStringLiteral("%weightNormal%"), d.weightNormal);
    addInt(QStringLiteral("%weightMedium%"), d.weightMedium);
    addInt(QStringLiteral("%weightSemiBold%"), d.weightSemiBold);
    addInt(QStringLiteral("%controlHeight%"), d.controlHeight);
    addInt(QStringLiteral("%controlHeightLg%"), d.controlHeightLg);
    addInt(QStringLiteral("%iconSize%"), d.iconSize);
    addInt(QStringLiteral("%scrollBarWidth%"), d.scrollBarWidth);
    addInt(QStringLiteral("%scrollBarThickness%"), d.scrollBarThickness);
    addInt(QStringLiteral("%splitterHandleWidth%"), d.splitterHandleWidth);
    addInt(QStringLiteral("%animFast%"), d.animFastMs);
    addInt(QStringLiteral("%animNormal%"), d.animNormalMs);

    return t;
}

const char *kStyleSheetTemplate = R"QSS(
/* ══════════════ 基础 ══════════════ */
QMainWindow {
    background-color: %background%;
}
QWidget#centralWidget {
    background-color: %background%;
}
QStackedWidget {
    background: transparent;
}
QStackedWidget > QWidget {
    background: transparent;
}
QWidget {
    color: %textPrimary%;
}
QLabel {
    color: %textPrimary%;
    background: transparent;
}

/* ══════════════ 侧边栏 ══════════════ */
QWidget#sidebar {
    background-color: %sidebar%;
    border: none;
    border-right: %borderWidth%px solid %border%;
}
QPushButton#sidebarToggle {
    color: %textPrimary%;
    background-color: %surface%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusCircle%px;
    padding: 0;
}
QPushButton#sidebarToggle:hover {
    background-color: %surfaceHover%;
    color: %textPrimary%;
}
QPushButton#navButton {
    color: %textSecondary%;
    background-color: transparent;
    border: none;
    border-left: %accentBarWidth%px solid transparent;
    border-radius: %radiusMd%px;
    font-size: %fontSizeMd%px;
    padding: 0;
}
QPushButton#navButton[sidebarExpanded="true"] {
    text-align: left;
    padding-left: %spaceMd%px;
    padding-right: %spaceSm%px;
}
QPushButton#navButton[sidebarExpanded="false"] {
    text-align: center;
    padding: 0;
}
QPushButton#navButton:hover {
    background-color: %surfaceHover%;
    color: %textPrimary%;
}
QPushButton#navButton:checked {
    background-color: %accentSubtle%;
    border-left-color: %accent%;
    color: %textPrimary%;
    font-weight: %weightSemiBold%;
}

/* ══════════════ 使用页 ══════════════ */
QPushButton#periodButton {
    color: %textSecondary%;
    background-color: transparent;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusMd%px;
    padding: 7px 22px;
    font-size: 13px;
    font-weight: %weightMedium%;
}
QPushButton#periodButton:hover:!checked {
    background-color: %accentSoft%;
}
QPushButton#periodButton:checked {
    background-color: %accent%;
    border-color: %accent%;
    color: %accentText%;
    font-weight: %weightSemiBold%;
}
QFrame#chartCard {
    background-color: %surface%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusXl%px;
}
QLabel#primaryStatLabel {
    color: %textPrimary%;
    font-size: %fontSizeMd%px;
}
QLabel#statsTitle {
    color: %textPrimary%;
    font-size: %fontSizeMd%px;
    font-weight: %weightSemiBold%;
}

/* ══════════════ 应用统计列表 ══════════════ */
QListWidget#appStatsList {
    background-color: transparent;
    border: none;
    border-radius: 0;
    color: %textPrimary%;
    padding: 0;
    outline: none;
}
QListWidget#appStatsList::item {
    background-color: transparent;
    border-bottom: %borderWidth%px solid %divider%;
    border-radius: 0;
    margin: 0;
    padding: 0;
}
QListWidget#appStatsList::item:hover {
    background-color: %surfaceHover%;
}
QListWidget#appStatsList::item:selected {
    background-color: %accentSoft%;
}

/* ══════════════ 分割条 ══════════════ */
QSplitter::handle:vertical {
    background-color: %splitterHandle%;
    margin: 6px 24px;
    border-radius: 1px;
}
QSplitter::handle:vertical:hover {
    background-color: %splitterHandleHover%;
}

/* ══════════════ 滚动条 ══════════════ */
QScrollArea {
    border: none;
    background: transparent;
}
QScrollArea > QWidget {
    background: transparent;
}
QScrollArea > QWidget > QWidget {
    background: transparent;
}
QScrollBar:vertical {
    background: transparent;
    width: %scrollBarWidth%px;
    margin: 0;
    border: none;
}
QScrollBar::handle:vertical {
    background: %scrollHandle%;
    border-radius: 4px;
    min-height: 36px;
    margin: 2px 3px;
}
QScrollBar::handle:vertical:hover {
    background: %scrollHandleHover%;
}
QScrollBar:horizontal {
    background: transparent;
    height: %scrollBarThickness%px;
    margin: 0;
    border: none;
}
QScrollBar::handle:horizontal {
    background: %scrollHandle%;
    border-radius: 4px;
    min-width: 36px;
    margin: 3px 2px;
}
QScrollBar::handle:horizontal:hover {
    background: %scrollHandleHover%;
}
QScrollBar::add-line, QScrollBar::sub-line {
    width: 0;
    height: 0;
    border: none;
    background: transparent;
}
QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

/* ══════════════ 设置页卡片 ══════════════ */
QFrame#settingsCard {
    background-color: %surface%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusLg%px;
}
QFrame#settingsCard QLabel {
    color: %textPrimary%;
    font-size: %fontSizeMd%px;
    background: transparent;
}
QFrame#settingsCard[settingsCardVariant="link"] QPushButton {
    color: %link%;
    background: transparent;
    border: none;
    text-align: left;
    padding: 0;
    font-size: %fontSizeMd%px;
}
QFrame#settingsCard[settingsCardVariant="link"] QPushButton:hover {
    color: %linkHover%;
}
QLabel#settingsPageTitle {
    font-size: %fontSizeXl%px;
    font-weight: %weightSemiBold%;
    color: %textPrimary%;
}
QLabel#settingsSectionTitle {
    font-size: %fontSizeLg%px;
    font-weight: %weightSemiBold%;
    margin-top: %spaceMd%px;
    color: %textPrimary%;
}
QLabel#settingsMutedLabel {
    font-size: %fontSizeMd%px;
    margin-top: %spaceSm%px;
    color: %textSecondary%;
}
QLabel#mutedValue {
    color: %textMuted%;
}

/* ══════════════ 通用控件 ══════════════ */
QComboBox {
    color: %textPrimary%;
    background-color: %inputBackground%;
    border: %borderWidth%px solid %inputBorder%;
    border-radius: %radiusMd%px;
    padding: 6px %spaceMd%px;
}
QComboBox:hover {
    border-color: %focus%;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 22px;
    border: none;
}
QComboBox::down-arrow {
    image: url("%comboArrow%");
    width: 12px;
    height: 12px;
}
QComboBox QAbstractItemView {
    color: %textPrimary%;
    background-color: %elevatedSurface%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusMd%px;
    selection-background-color: %accentSoft%;
    outline: none;
}
QLineEdit {
    color: %textPrimary%;
    background-color: %inputBackground%;
    border: %borderWidth%px solid %inputBorder%;
    border-radius: %radiusMd%px;
    padding: 8px %spaceMd%px;
}
QLineEdit:focus {
    border-color: %focus%;
}
QTextEdit {
    background-color: %inputBackground%;
    color: %textSecondary%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusLg%px;
    padding: %spaceMd%px;
    font-size: 13px;
}
QPushButton {
    color: %textPrimary%;
    background-color: %surfaceHover%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusMd%px;
    padding: 8px 20px;
    font-size: 13px;
}
QPushButton:hover {
    background-color: %surfacePressed%;
}
QPushButton:pressed {
    background-color: %surfacePressed%;
}
QPushButton:disabled {
    background-color: %disabledBackground%;
    color: %textDisabled%;
}
QPushButton#primaryButton {
    color: %accentText%;
    background-color: %accent%;
    border: none;
    border-radius: %radiusMd%px;
    padding: 8px 20px;
    font-size: %fontSizeMd%px;
    font-weight: %weightSemiBold%;
}
QPushButton#primaryButton:hover {
    background-color: %accentHover%;
}
QPushButton#primaryButton:pressed {
    background-color: %accentPressed%;
}
QPushButton#primaryButton:disabled {
    background-color: %disabledBackground%;
    color: %textDisabled%;
}
QToolButton {
    background: transparent;
    border: none;
    border-radius: %radiusSm%px;
    padding: 4px;
}
QToolButton:hover {
    background-color: %surfaceHover%;
}
QSlider::groove:horizontal {
    height: 4px;
    background: %divider%;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: %accent%;
    border-radius: 2px;
}
QSlider::add-page:horizontal {
    background: %divider%;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    width: 14px;
    margin: -6px 0;
    border-radius: 7px;
    background: %accent%;
}
QSlider::handle:horizontal:hover {
    background: %accentHover%;
}

/* ══════════════ 浮层 / 弹窗 ══════════════ */
QMenu {
    background-color: %elevatedSurface%;
    color: %textPrimary%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusMd%px;
    padding: 4px;
}
QMenu::item {
    padding: 8px 20px;
    border-radius: %radiusXs%px;
}
QMenu::item:selected {
    background-color: %accentSoft%;
    color: %textPrimary%;
}
QMenu::separator {
    height: %borderWidth%px;
    background: %divider%;
    margin: 4px 8px;
}
QToolTip {
    background-color: %tooltipBackground%;
    color: %tooltipText%;
    border: %borderWidth%px solid %tooltipBorder%;
    border-radius: %radiusSm%px;
    padding: 6px 8px;
}
QMessageBox, QDialog {
    background-color: %surface%;
}
QMessageBox QLabel {
    color: %textPrimary%;
}
QMessageBox QPushButton {
    min-width: 72px;
}

/* ══════════════ AI 分析报告页 ══════════════ */
QLabel#pageTitle {
    font-size: %fontSizeXl%px;
    font-weight: %weightSemiBold%;
    color: %textPrimary%;
}
QLabel#aiReportSubtitle {
    color: %textMuted%;
    font-size: 13px;
    margin-bottom: 4px;
}
QLabel#aiReportStatus {
    color: %textMuted%;
}
QFrame#aiReportButtonRow {
    background-color: %surface%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusLg%px;
}
QFrame#aiReportBlock {
    background-color: %surface%;
    border: %borderWidth%px solid %border%;
    border-radius: %radiusLg%px;
}
QLabel#aiReportBlockTitle {
    font-size: 15px;
    font-weight: %weightSemiBold%;
    color: %accent%;
}
QLabel#aiReportBlockContent {
    font-size: %fontSizeMd%px;
    color: %textPrimary%;
    padding: 4px 0;
}
QLabel#aiReportAutoTag {
    font-size: %fontSizeSm%px;
    color: %textMuted%;
}
QPushButton#deleteReportButton {
    color: %textMuted%;
    background-color: transparent;
    border: none;
    font-size: 16px;
    padding: 0 6px;
    min-width: 24px;
    min-height: 24px;
}
QPushButton#deleteReportButton:hover {
    color: %error%;
}
)QSS";

} // namespace

namespace ThemeStyles {

QString colorValue(const QColor &color)
{
    if (color.alpha() >= 255) {
        return color.name(QColor::HexRgb);
    }
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QPalette palette(const Theme &theme)
{
    const ColorTokens &c = theme.colors;
    QPalette p;

    p.setColor(QPalette::Window, c.background);
    p.setColor(QPalette::WindowText, c.textPrimary);
    p.setColor(QPalette::Base, c.surface);
    p.setColor(QPalette::AlternateBase, c.elevatedSurface);
    p.setColor(QPalette::Text, c.textPrimary);
    p.setColor(QPalette::Button, c.surface);
    p.setColor(QPalette::ButtonText, c.textPrimary);
    p.setColor(QPalette::BrightText, c.error);
    p.setColor(QPalette::Highlight, c.accent);
    p.setColor(QPalette::HighlightedText, c.accentText);
    p.setColor(QPalette::ToolTipBase, c.tooltipBackground);
    p.setColor(QPalette::ToolTipText, c.tooltipText);
    p.setColor(QPalette::PlaceholderText, c.textMuted);
    p.setColor(QPalette::Link, c.link);
    p.setColor(QPalette::LinkVisited, c.link);
    p.setColor(QPalette::Mid, c.divider);
    p.setColor(QPalette::Dark, c.border);
    p.setColor(QPalette::Shadow, c.border);

    p.setColor(QPalette::Disabled, QPalette::WindowText, c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::Text, c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::Highlight, c.disabledBackground);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, c.textDisabled);
    return p;
}

QString globalStyleSheet(const Theme &theme)
{
    return applyTokens(QString::fromUtf8(kStyleSheetTemplate), buildTokenMap(theme));
}

QIcon svgIcon(const QString &svgTemplate, const QColor &color, const QSize &size)
{
    QString svg = svgTemplate;
    svg.replace(QStringLiteral("currentColor"), color.name(QColor::HexRgb));

    QSvgRenderer renderer(svg.toUtf8());
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    return QIcon(pixmap);
}

} // namespace ThemeStyles
