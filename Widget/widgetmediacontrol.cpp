#include "widgetmediacontrol.h"
#include "ui_widgetmediacontrol.h"

WidgetMediaControl::WidgetMediaControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMediaControl)
{
    ui->setupUi(this);
}

WidgetMediaControl::~WidgetMediaControl()
{
    delete ui;
}
