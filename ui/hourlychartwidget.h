#ifndef HOURLYCHARTWIDGET_H
#define HOURLYCHARTWIDGET_H

#include <QMap>
#include <QPair>
#include <QStringList>
#include <QVector>
#include <QWidget>

class HourlyChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HourlyChartWidget(QWidget *parent = nullptr);

    void setDailyData(const QVector<int> &minutesByHour,
                      const QMap<int, QVector<QPair<QString, int>>> &topAppsByHour);
    void setChartData(const QVector<int> &minutesByBucket,
                      const QStringList &bucketLabels,
                      const QMap<int, QVector<QPair<QString, int>>> &topAppsByBucket,
                      bool hourRangeTooltip,
                      int fixedMaxMinutes = -1,
                      int tickIntervalMinutes = 30);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QRect chartRect() const;
    int hoverBucketAtPosition(const QPoint &position) const;

    QVector<int> m_minutesByBucket;
    QStringList m_bucketLabels;
    QMap<int, QVector<QPair<QString, int>>> m_topAppsByBucket;
    int m_hoverBucket = -1;
    bool m_hourRangeTooltip = false;
    int m_fixedMaxMinutes = -1;
    int m_tickIntervalMinutes = 30;
};

#endif // HOURLYCHARTWIDGET_H
