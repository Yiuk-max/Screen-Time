#include "hourlychartwidget.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

static QString formatDurationFromMinutes(int minutes)
{
    const int hours = minutes / 60;
    const int mins = minutes % 60;
    return QStringLiteral("%1小时%2分钟").arg(hours).arg(mins);
}

HourlyChartWidget::HourlyChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(280);
}

void HourlyChartWidget::setDailyData(const QVector<int> &minutesByHour,
                                     const QMap<int, QVector<QPair<QString, int>>> &topAppsByHour)
{
    setChartData(minutesByHour, {}, topAppsByHour, {}, true);
}

void HourlyChartWidget::setChartData(const QVector<int> &minutesByBucket,
                                     const QStringList &bucketLabels,
                                     const QMap<int, QVector<QPair<QString, int>>> &topAppsByBucket,
                                     const QMap<QString, QIcon> &appIconsByName,
                                     bool hourRangeTooltip,
                                     int fixedMaxMinutes,
                                     int tickIntervalMinutes)
{
    m_minutesByBucket = minutesByBucket;
    m_bucketLabels = bucketLabels;
    m_topAppsByBucket = topAppsByBucket;
    m_appIconsByName = appIconsByName;
    m_hourRangeTooltip = hourRangeTooltip;
    m_fixedMaxMinutes = fixedMaxMinutes;
    m_tickIntervalMinutes = qMax(1, tickIntervalMinutes);
    update();
}

void HourlyChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), QColor(28, 28, 30));

    const QRect area = chartRect();
    if (area.width() <= 0 || area.height() <= 0) {
        return;
    }

    const QColor gridColor(78, 78, 82);
    const QColor axisTextColor(150, 150, 156);
    const QColor barColor(45, 135, 255);
    int maxMinutes = (m_fixedMaxMinutes > 0) ? m_fixedMaxMinutes : 30;
    if (m_fixedMaxMinutes <= 0) {
        for (int value : m_minutesByBucket) {
            maxMinutes = qMax(maxMinutes, value);
        }
        maxMinutes = ((maxMinutes + 29) / 30) * 30;
    }

    QPen gridPen(gridColor);
    gridPen.setStyle(Qt::DashLine);
    gridPen.setWidth(1);
    p.setPen(gridPen);

    QVector<int> yTicks;
    for (int tick = 0; tick <= maxMinutes; tick += m_tickIntervalMinutes) {
        yTicks.append(tick);
    }
    if (yTicks.isEmpty() || yTicks.last() != maxMinutes) {
        yTicks.append(maxMinutes);
    }
    for (int minute : yTicks) {
        const double t = static_cast<double>(minute) / maxMinutes;
        const int y = area.bottom() - static_cast<int>(t * area.height());
        p.drawLine(area.left(), y, area.right(), y);

        p.setPen(axisTextColor);
        p.drawText(area.right() + 8, y + 5, QStringLiteral("%1 分钟").arg(minute));
        p.setPen(gridPen);
    }

    p.setPen(axisTextColor);
    const int bucketCount = m_minutesByBucket.isEmpty() ? 1 : m_minutesByBucket.size();
    const double slotWidth = static_cast<double>(area.width()) / bucketCount;
    for (int i = 0; i < bucketCount; ++i) {
        if (bucketCount > 12 && i % 3 != 0) {
            continue;
        }
        const int x = area.left() + static_cast<int>((i + 0.5) * slotWidth);
        QString label = (i < m_bucketLabels.size()) ? m_bucketLabels[i] : QString::number(i);
        p.drawText(x - 18, area.bottom() + 28, label);
    }

    for (int i = 0; i < bucketCount; ++i) {
        const int minutes = (i < m_minutesByBucket.size()) ? m_minutesByBucket[i] : 0;
        if (minutes <= 0) {
            continue;
        }

        const double ratio = qMin(1.0, static_cast<double>(minutes) / maxMinutes);
        const int barHeight = static_cast<int>(ratio * area.height());
        const int x = area.left() + static_cast<int>(i * slotWidth + slotWidth * 0.2);
        const int w = qMax(6, static_cast<int>(slotWidth * 0.6));
        const int y = area.bottom() - barHeight;

        QRect barRect(x, y, w, barHeight);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(barRect, 4, 4);
    }

    if (m_hoverBucket >= 0 && m_hoverBucket < bucketCount) {
        const double x0d = area.left() + m_hoverBucket * slotWidth;
        const double x1d = x0d + slotWidth;
        const int x0 = static_cast<int>(x0d);
        const int x1 = static_cast<int>(x1d);

        p.setPen(QPen(QColor(110, 110, 115), 1, Qt::DashLine));
        p.drawLine(x0, area.top(), x0, area.bottom());
        p.drawLine(x1, area.top(), x1, area.bottom());

        const int minutes = (m_hoverBucket < m_minutesByBucket.size()) ? m_minutesByBucket[m_hoverBucket] : 0;
        const auto topApps = m_topAppsByBucket.value(m_hoverBucket);
        const int shownTop = qMin(3, topApps.size());
        const int tipHeight = 52 + shownTop * 24;
        QRect tipRect(area.left() + area.width() / 2 - 170, area.top() + 8, 340, tipHeight);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(56, 56, 60, 230));
        p.drawRoundedRect(tipRect, 12, 12);

        p.setPen(QColor(235, 235, 240));
        QString summary = QStringLiteral("总时长 %1").arg(formatDurationFromMinutes(minutes));
        if (m_hourRangeTooltip) {
            summary += QStringLiteral("  (%1时-%2时)").arg(m_hoverBucket).arg(m_hoverBucket + 1);
        }
        p.drawText(tipRect.adjusted(12, 10, -12, -10), Qt::AlignLeft | Qt::AlignTop, summary);

        int y = tipRect.top() + 34;
        for (int i = 0; i < shownTop; ++i) {
            const QString &appName = topApps[i].first;
            const int appMinutes = topApps[i].second;
            const QRect iconRect(tipRect.left() + 12, y, 16, 16);
            const QIcon icon = m_appIconsByName.value(appName);
            if (!icon.isNull()) {
                icon.paint(&p, iconRect);
            } else {
                p.setBrush(QColor(120, 120, 126));
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(iconRect, 3, 3);
                p.setPen(QColor(235, 235, 240));
            }
            p.drawText(tipRect.left() + 34, y + 13,
                       QStringLiteral("%1  %2").arg(appName, formatDurationFromMinutes(appMinutes)));
            y += 24;
        }
    }
}

void HourlyChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    const int nextBucket = hoverBucketAtPosition(event->pos());
    if (nextBucket != m_hoverBucket) {
        m_hoverBucket = nextBucket;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void HourlyChartWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (m_hoverBucket != -1) {
        m_hoverBucket = -1;
        update();
    }
}

QRect HourlyChartWidget::chartRect() const
{
    return rect().adjusted(30, 20, -110, -40);
}

int HourlyChartWidget::hoverBucketAtPosition(const QPoint &position) const
{
    const QRect area = chartRect();
    if (!area.contains(position)) {
        return -1;
    }
    const int bucketCount = m_minutesByBucket.isEmpty() ? 1 : m_minutesByBucket.size();
    const double slotWidth = static_cast<double>(area.width()) / bucketCount;
    const int bucket = static_cast<int>((position.x() - area.left()) / slotWidth);
    if (bucket < 0 || bucket >= bucketCount) {
        return -1;
    }
    return bucket;
}
