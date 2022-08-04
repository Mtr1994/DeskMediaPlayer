#ifndef WIDGETMEDIACONTROL_H
#define WIDGETMEDIACONTROL_H

#include <QWidget>

namespace Ui {
class WidgetMediaControl;
}

class WidgetMediaControl : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMediaControl(QWidget *parent = nullptr);
    ~WidgetMediaControl();

private:
    Ui::WidgetMediaControl *ui;
};

#endif // WIDGETMEDIACONTROL_H
