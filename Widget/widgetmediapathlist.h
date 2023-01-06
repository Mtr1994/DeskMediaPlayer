#ifndef WIDGETMEDIAPATHLIST_H
#define WIDGETMEDIAPATHLIST_H

#include <QWidget>
#include <QStandardItemModel>

namespace Ui {
class WidgetMediaPathList;
}

class WidgetMediaPathList : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMediaPathList(QWidget *parent = nullptr);
    ~WidgetMediaPathList();

private:
    void init();

public:
    void setMediaPaths(const QStringList &list);

    void setCurrentMediaPaths(const QString &path);

private slots:
    void slot_list_view_double_click(const QModelIndex &index);

private:
    Ui::WidgetMediaPathList *ui;

    QStandardItemModel mModelMediaPath;
};

#endif // WIDGETMEDIAPATHLIST_H
