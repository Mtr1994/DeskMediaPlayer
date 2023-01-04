#include "widgetmediacontrol.h"
#include "ui_widgetmediacontrol.h"
#include "Public/appsignal.h"
#include "Configure/softconfig.h"

#include <thread>

// test
#include <QDebug>

WidgetMediaControl::WidgetMediaControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMediaControl)
{
    ui->setupUi(this);

    init();
}

WidgetMediaControl::~WidgetMediaControl()
{
    // 记录音量配置
    SoftConfig::getInstance()->setValue("Volume", "value", QString::number(ui->sliderVolume->value()));

    delete ui;
}

void WidgetMediaControl::init()
{
    connect(ui->btnPreviousFrame, &QPushButton::clicked, this, &WidgetMediaControl::slot_btn_play_previous_frame);
    connect(ui->btnPlay, &QPushButton::clicked, this, &WidgetMediaControl::slot_btn_play_click);
    connect(ui->btnNextFrame, &QPushButton::clicked, this, &WidgetMediaControl::slot_btn_play_next_frame);

    // 全屏显示
    connect(ui->btnFullScreen, &QPushButton::clicked, AppSignal::getInstance(), &AppSignal::sgl_media_show_full_screen);

    connect(ui->sliderVolume, &QSlider::valueChanged, this, &WidgetMediaControl::slot_volume_value_change);
    // 读取音量配置
    ui->sliderVolume->setValue(SoftConfig::getInstance()->getValue("Volume", "value").toUInt());

    connect(ui->slider, &QSlider::sliderPressed, this, [this]{ ui->slider->setSliderDown(true); });
    connect(ui->slider, &QSlider::sliderReleased, this, [this]{ ui->slider->setSliderDown(false); });
    connect(ui->slider, &QSlider::sliderMoved, this, [](int position)
    {
        emit AppSignal::getInstance()->sgl_seek_video_position(position);
    });

    connect(AppSignal::getInstance(), &AppSignal::sgl_init_media_duration, this, &WidgetMediaControl::slot_init_media_duration);
    connect(AppSignal::getInstance(), &AppSignal::sgl_thread_current_video_frame_time, this, &WidgetMediaControl::slot_thread_current_video_frame_time, Qt::QueuedConnection);
    connect(AppSignal::getInstance(), &AppSignal::sgl_thread_finish_play_video, this, &WidgetMediaControl::slot_thread_finish_play_video, Qt::QueuedConnection);
}

void WidgetMediaControl::changePlayStatus()
{
    ui->btnPlay->setChecked(!ui->btnPlay->isChecked());
    slot_btn_play_click();
}

void WidgetMediaControl::slot_btn_play_previous_frame()
{
    int targetPosition = ui->slider->value() - 5.0 / mMediaTimeBase;
    if (targetPosition <= 0) targetPosition = 0;

    emit AppSignal::getInstance()->sgl_seek_video_position(targetPosition);
}

void WidgetMediaControl::slot_btn_play_click()
{
    if (ui->btnPlay->isChecked())
    {
        emit AppSignal::getInstance()->sgl_start_play_video();
    }
    else
    {
        emit AppSignal::getInstance()->sgl_pause_play_video();
    }
}

void WidgetMediaControl::slot_btn_play_next_frame()
{
    int targetPosition = ui->slider->value() + 5.0 / mMediaTimeBase;
    if (targetPosition >= ui->slider->maximum()) targetPosition = ui->slider->maximum();
    emit AppSignal::getInstance()->sgl_seek_video_position(targetPosition);
}

void WidgetMediaControl::slot_volume_value_change(int volume)
{
    emit AppSignal::getInstance()->sgl_change_audio_volume(volume);
}

void WidgetMediaControl::slot_init_media_duration(int64_t duration, double timebase)
{
    if (duration < 0)
    {
        ui->lbDuration->setText("00:00:00");
        ui->lbDurationLeft->setText("00:00:00");
        ui->slider->setValue(ui->slider->maximum());
        return;
    }

    ui->slider->setRange(0, duration);

    qDebug() << "init duration " << duration << " " << timebase;
    qDebug() << "init time " << duration * timebase;

    ui->btnPlay->setChecked(true);

    mMediaTimeBase = timebase;
    mMediaBaseDuration = duration * timebase;
    ui->lbDurationLeft->setText(QString("%1:%2:%3").arg(mMediaBaseDuration / 3600, 2, 10, QLatin1Char('0')).arg(mMediaBaseDuration / 60, 2, 10, QLatin1Char('0')).arg(mMediaBaseDuration % 60, 2, 10, QLatin1Char('0')));
}

void WidgetMediaControl::slot_thread_current_video_frame_time(int64_t pts, float timebase)
{
    if (mMediaBaseDuration < 0) return;

    if (!ui->slider->isSliderDown())
    {
        ui->slider->setValue(pts / mMediaTimeBase * timebase);
    }

    uint32_t time = pts * timebase;
    ui->lbDuration->setText(QString("%1:%2:%3").arg(uint32_t(time) / 3600, 2, 10, QLatin1Char('0')).arg(uint32_t(time) % 3600 / 60, 2, 10, QLatin1Char('0')).arg(uint32_t(time) % 60, 2, 10, QLatin1Char('0')));

    uint32_t leftTime = mMediaBaseDuration - time;
    ui->lbDurationLeft->setText(QString("%1:%2:%3").arg(leftTime / 3600, 2, 10, QLatin1Char('0')).arg(leftTime % 3600 / 60, 2, 10, QLatin1Char('0')).arg(leftTime % 60, 2, 10, QLatin1Char('0')));
}

void WidgetMediaControl::slot_thread_finish_play_video()
{
    slot_thread_current_video_frame_time(ui->slider->maximum(), mMediaTimeBase);
}
