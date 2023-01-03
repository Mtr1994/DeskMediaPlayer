#ifndef DIALOGVERSION_H
#define DIALOGVERSION_H

#include <QDialog>

namespace Ui {
class DialogVersion;
}

class DialogVersion : public QDialog
{
    Q_OBJECT

public:
    explicit DialogVersion(QWidget *parent = nullptr);
    ~DialogVersion();

    void init();

private:
    Ui::DialogVersion *ui;
};

#endif // DIALOGVERSION_H
