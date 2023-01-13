#ifndef WIDGETDRAWER_H
#define WIDGETDRAWER_H

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEvent>

class QPropertyAnimation;
class WidgetDrawer : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetDrawer(QWidget *parent = nullptr);
    ~WidgetDrawer();

    void setContentWidget(QWidget* widget);

    void showDrawer();
    void closeDrawer();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void init();

private slots:
    void slot_animation_value_change(const QVariant &value);

signals:

private:
    QWidget *mParentWidget = nullptr;
    QWidget *mInputWidget = nullptr;

    QColor mBackgroundColor = { 0, 0, 0, 114 };

    // 标题
    QWidget *mTitleWidget = nullptr;

    // 内容
    QWidget *mContentWidget = nullptr;

    // 内容容器
    QWidget *mContainerWidget = nullptr;

    // 内容容器过渡动画
    QPropertyAnimation *mAnimation = nullptr;

    // 抽屉宽度
    uint32_t mDrawerWidth = 0;

    // 标题文本
    QString mTitleText = "播放列表";
};

#endif // WIDGETDRAWER_H
