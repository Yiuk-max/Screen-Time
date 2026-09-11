#include "fluenttoggleswitch.h"

#include "theme/thememanager.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QEasingCurve>

FluentToggleSwitch::FluentToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
{
    // 注意：不设置 setCheckable(true) — 我们自己管理 checked 状态，
    // 避免 QAbstractButton 的自动切换逻辑干扰
    setCursor(Qt::PointingHandCursor);
    setFixedSize(44, 24);

    m_animation = new QPropertyAnimation(this, "thumbPos", this);
    m_animation->setDuration(ThemeManager::instance().theme().design.animNormalMs);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // 颜色随主题变化：切换后自动重绘。
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
            [this](const Theme &) { update(); });
}

QSize FluentToggleSwitch::sizeHint() const
{
    return QSize(44, 24);
}

// ── slot ──────────────────────────────────────────────────────────
void FluentToggleSwitch::setChecked(bool checked, bool animated)
{
    m_checked = checked;
    if (animated && isVisible()) {
        animateTo(checked);
    } else {
        m_thumbPos = checked ? 1.0 : 0.0;
        update();
    }
}

// ── property ──────────────────────────────────────────────────────
void FluentToggleSwitch::setThumbPos(double pos)
{
    m_thumbPos = qBound(0.0, pos, 1.0);
    update();
}

void FluentToggleSwitch::animateTo(bool checked)
{
    m_animation->stop();
    m_animation->setStartValue(m_thumbPos);
    m_animation->setEndValue(checked ? 1.0 : 0.0);
    m_animation->start();
}

QRectF FluentToggleSwitch::thumbRect() const
{
    const double h = height();
    const double pad = 3.0;
    const double thumbSize = h - pad * 2.0;           // 18px
    const double trackW = width() - pad * 2.0;        // 38px
    const double x = pad + m_thumbPos * (trackW - thumbSize);
    return QRectF(x, pad, thumbSize, thumbSize);
}

void FluentToggleSwitch::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const double h = height();
    const double w = width();
    const double radius = h / 2.0;

    const ColorTokens &c = ThemeManager::instance().theme().colors;
    const QColor cOff = c.textMuted;
    const QColor cOn = c.accent;

    // 轨道颜色根据 m_thumbPos 线性插值
    const double t = m_thumbPos;
    const int r = static_cast<int>(cOff.red()   + (cOn.red()   - cOff.red())   * t);
    const int g = static_cast<int>(cOff.green() + (cOn.green() - cOff.green()) * t);
    const int b = static_cast<int>(cOff.blue()  + (cOn.blue()  - cOff.blue())  * t);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(r, g, b));
    p.drawRoundedRect(QRectF(0, 0, w, h), radius, radius);

    // 滑块
    p.setBrush(c.accentText);
    p.drawEllipse(thumbRect());
}

void FluentToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->pos())) {
        m_checked = !m_checked;
        animateTo(m_checked);
        emit toggled(m_checked);
        return;  // ↵ 不调用基类，完全自己控制
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void FluentToggleSwitch::resizeEvent(QResizeEvent *event)
{
    QAbstractButton::resizeEvent(event);
    m_thumbPos = m_checked ? 1.0 : 0.0;
}
