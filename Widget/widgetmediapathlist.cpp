#include "widgetmediapathlist.h"
#include "ui_widgetmediapathlist.h"
#include "Public/appsignal.h"

#include <QFileInfo>
#include <QStandardItem>

WidgetMediaPathList::WidgetMediaPathList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMediaPathList)
{
    ui->setupUi(this);

    init();
}

WidgetMediaPathList::~WidgetMediaPathList()
{
    delete ui;
}

void WidgetMediaPathList::init()
{
    ui->lvMediaPathList->setModel(&mModelMediaPath);

    connect(ui->lvMediaPathList, &QListView::doubleClicked, this, &WidgetMediaPathList::slot_list_view_double_click);
}

void WidgetMediaPathList::setMediaPaths(const QStringList &list)
{
    mModelMediaPath.clear();
    for (auto &path : list)
    {
        QFileInfo info(path);
        QStandardItem *item = new QStandardItem(info.fileName());
        item->setData(info.absoluteFilePath(), Qt::UserRole + 1);
        item->setWhatsThis(info.absoluteFilePath());
        mModelMediaPath.appendRow(item);
    }
}

void WidgetMediaPathList::setCurrentMediaPaths(const QString &path)
{
    if (path.isEmpty()) return;
    QModelIndexList list = mModelMediaPath.match(mModelMediaPath.index(0, 0), Qt::UserRole + 1, path, 1, Qt::MatchExactly);
    if (list.size() == 0) return;

    ui->lvMediaPathList->setCurrentIndex(list.first());
}

void WidgetMediaPathList::slot_list_view_double_click(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QStandardItem *item = mModelMediaPath.itemFromIndex(index);
    if (nullptr == item) return;

    emit AppSignal::getInstance()->sgl_start_play_target_media(item->data(Qt::UserRole + 1).toString());
}
