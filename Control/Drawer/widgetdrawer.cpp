#include "widgetdrawer.h"

#include <QGridLayout>
#include <QPropertyAnimation>
#include <QPainter>
#include <QRect>
#include <QFontMetrics>
#include <QFont>
#include <QLabel>
#include <QTimer>

// test
#include <QDebug>
#include <QDateTime>

WidgetDrawer::WidgetDrawer(QWidget *parent)
    : QWidget{parent}, mParentWidget(parent)
{
    init();
}

WidgetDrawer::~WidgetDrawer()
{
    delete mAnimation;
    qDebug() << "WidgetDrawer::~WidgetDrawer";
}

void WidgetDrawer::setContentWidget(QWidget *widget)
{
    mInputWidget = widget;
    mContainerWidget->layout()->addWidget(mInputWidget);
}

void WidgetDrawer::showDrawer()
{
    setVisible(true);
    mAnimation->start();
}

void WidgetDrawer::closeDrawer()
{
    mAnimation->setEasingCurve(QEasingCurve::InQuad);
    mAnimation->setDirection(QPropertyAnimation::Backward);
    mAnimation->start();
}

void WidgetDrawer::paintEvent(QPaintEvent *event)
{
    if (nullptr != mParentWidget)
    {
        setGeometry(0, 0, mParentWidget->width(), mParentWidget->height());
        mContentWidget->resize(mDrawerWidth, mParentWidget->height());
        mContainerWidget->setGeometry(0, mTitleWidget->height(), mContentWidget->width(), mContentWidget->height() - mTitleWidget->height());
    }

    bool isHide = (mAnimation->direction() == QPropertyAnimation::Backward);

    QPainter *painter = new QPainter;
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(isHide ? QColor(0, 0, 0, 0) : mBackgroundColor));

    int width = this->width();
    int height = this->height();

    // 绘制背景
    painter->drawRect(QRect(mContentWidget->width(), 0, width, height));

    painter->end();

    delete painter;

    QWidget::paintEvent(event);
}

void WidgetDrawer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    if (event->pos().x() <= (int)mDrawerWidth) return;
    QWidget::mousePressEvent(event);
    closeDrawer();
}

void WidgetDrawer::init()
{
    if (nullptr == mParentWidget) return;

    mContentWidget = new QWidget(this);
    mContentWidget->setObjectName("contentWidget");
    mContentWidget->setStyleSheet("QWidget#contentWidget{ background-color: #fefefe; border: none; }");

    QFont font("Microsoft YaHei", 10);
    QFontMetrics metrics(font);
    QRect fontRect = metrics.boundingRect("测试文本");
    mDrawerWidth = fontRect.width() * 6;

    mContentWidget->setGeometry(mDrawerWidth * -1, 0, mDrawerWidth, mParentWidget->height());

    // 标题
    mTitleWidget = new QWidget(mContentWidget);
    mTitleWidget->setGeometry(0, 0, mContentWidget->width(), fontRect.height() * 3);
    mTitleWidget->setObjectName("titleWidget");
    mTitleWidget->setStyleSheet("QWidget#titleWidget{ background-color: #fefefe; border: none; border-bottom: 1px solid #e8e8e8; border-top: 1px solid #e8e8e8}");

    QGridLayout *layout = new QGridLayout(mTitleWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    // 标题文本
    QLabel *label = new QLabel(mTitleText, mTitleWidget);
    label->setStyleSheet("QLabel{color: #d9000000; padding-left: 1em; font: 480 12pt 'Microsoft YaHei';}");
    layout->addWidget(label);

    // 内置内容
    mContainerWidget = new QWidget(mContentWidget);
    mContainerWidget->setObjectName("containerWidget");
    mContainerWidget->setStyleSheet("QWidget#containerWidget{ background-color: #fefefe;}");

    QGridLayout *layoutMain = new QGridLayout(mContainerWidget);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    // 过渡动画
    mAnimation = new QPropertyAnimation(mContentWidget, "pos", this);
    mAnimation->setDuration(220);
    mAnimation->setEasingCurve(QEasingCurve::OutQuad);
    mAnimation->setStartValue(QPoint(mDrawerWidth * -1, 0));
    mAnimation->setEndValue(QPoint(0, 0));

    connect(mAnimation, &QPropertyAnimation::valueChanged, this, &WidgetDrawer::slot_animation_value_change);
}

void WidgetDrawer::slot_animation_value_change(const QVariant &value)
{
    Q_UNUSED(value);
    if (nullptr == mAnimation) return;

    int x = mDrawerWidth * -1;
    if (value.toPoint().x() == x)
    {
        if (mAnimation->direction() ==  QPropertyAnimation::Backward)
        {
            disconnect(mAnimation, &QPropertyAnimation::valueChanged, this, &WidgetDrawer::slot_animation_value_change);
            setVisible(false);
            // 关闭直接析构
            this->deleteLater();
            return;
        }
    }

    update();
}
