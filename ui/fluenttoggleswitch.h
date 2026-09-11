#ifndef FLUENTTOGGLESWITCH_H
#define FLUENTTOGGLESWITCH_H

#include <QAbstractButton>
#include <QPropertyAnimation>
#include <QPainter>

class FluentToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(double thumbPos READ thumbPos WRITE setThumbPos)
public:
    explicit FluentToggleSwitch(QWidget *parent = nullptr);

    double thumbPos() const { return m_thumbPos; }
    bool isChecked() const { return m_checked; }

    QSize sizeHint() const override;

public slots:
    /// 设置选中状态，带动画（如果 visible）
    void setChecked(bool checked, bool animated = true);

signals:
    /// 用户交互触发的切换（区别于程序调用 setChecked）
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setThumbPos(double pos);
    void animateTo(bool checked);
    QRectF thumbRect() const;

    QPropertyAnimation *m_animation = nullptr;
    double m_thumbPos = 0.0;  // 0.0 = left, 1.0 = right
    bool m_checked = false;
};

#endif // FLUENTTOGGLESWITCH_H
