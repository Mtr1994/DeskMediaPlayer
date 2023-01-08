#include "widgetslider.h"

// Qdebug
#include <QDebug>
#include <QMouseEvent>
#include <QFontMetrics>

WidgetSlider::WidgetSlider(QWidget *parent)
    : QSlider{parent}
{
    init();
}

void WidgetSlider::init()
{
    setOrientation(Qt::Horizontal);
    setRange(0, 100);

    QFontMetrics metrics(QFont("Microsoft YaHei", 9));
    mGrooveMargin = 2;

    qDebug() << "mGrooveMargin ";
}

void WidgetSlider::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    mMousePressFlag = true;
}

void WidgetSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (!mMousePressFlag) return;
    setNewValue(event);
}

void WidgetSlider::mouseReleaseEvent(QMouseEvent *event)
{
    mMousePressFlag = false;
    setNewValue(event);
}

void WidgetSlider::setNewValue(QMouseEvent *event)
{
    float position = event->pos().x();

    int value = position / (width() - 1) * maximum();

    if (value < minimum()) value = minimum();
    if (value > maximum()) value = maximum();
    setValue(value);

    emit sgl_current_value_changed(value);
}
