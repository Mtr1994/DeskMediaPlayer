#include "widgetslider.h"

#include <QMouseEvent>

WidgetSlider::WidgetSlider(QWidget *parent)
    : QSlider{parent}
{
    init();
}

void WidgetSlider::init()
{
    setOrientation(Qt::Horizontal);
}

void WidgetSlider::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    mMousePressFlag = true;
    emit QSlider::sliderPressed();
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
    emit QSlider::sliderReleased();
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
