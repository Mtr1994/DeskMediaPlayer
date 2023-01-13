#include "dialogversion.h"
#include "ui_dialogversion.h"

DialogVersion::DialogVersion(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogVersion)
{
    ui->setupUi(this);

    init();
}

DialogVersion::~DialogVersion()
{
    delete ui;
}

void DialogVersion::init()
{
    setWindowTitle("版本信息");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    QStringList lst = QApplication::applicationVersion().split('.');
    if (lst.count() < 3)
    {
        ui->lbVersion->setText("--");
    }
    else
    {
        ui->lbVersion->setText(QString("00" + lst.at(0)).right(2) + "." +
                               QString("00" + lst.at(1)).right(2) + "." +
                               QString("00" + lst.at(2)).right(2) + "." +
                               QString("00" + lst.at(3)).right(4));
    }
}
