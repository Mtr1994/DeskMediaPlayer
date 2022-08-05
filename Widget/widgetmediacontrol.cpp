#include "widgetmediacontrol.h"
#include "ui_widgetmediacontrol.h"
#include "Public/appsignal.h"
#include "Configure/softconfig.h"

#include <thread>

WidgetMediaControl::WidgetMediaControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMediaControl)
{
    ui->setupUi(this);

    init();
}

WidgetMediaControl::~WidgetMediaControl()
{
    delete ui;
}

void WidgetMediaControl::init()
{
    connect(ui->btnPlay, &QPushButton::clicked, this, &WidgetMediaControl::slot_btn_play_click);

    connect(ui->sliderVolume, &QSlider::valueChanged, this, &WidgetMediaControl::slot_volume_value_change);
    connect(ui->sliderVolume, &QSlider::sliderReleased, this, [this]
    {
        auto func = [this]
        {
            // 记录音量配置
            SoftConfig::getInstance()->setValue("Volume", "value", QString::number(ui->sliderVolume->value()));
        };

        std::thread th(func);
        th.detach();
    });

    // 读取音量配置
    ui->sliderVolume->setValue(SoftConfig::getInstance()->getValue("Volume", "value").toUInt());

    connect(AppSignal::getInstance(), &AppSignal::sgl_get_media_duration, this, &WidgetMediaControl::slot_get_media_duration);
    connect(AppSignal::getInstance(), &AppSignal::sgl_current_video_frame_time, this, &WidgetMediaControl::slot_current_video_frame_time);
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

void WidgetMediaControl::slot_volume_value_change(int volume)
{


    emit AppSignal::getInstance()->sgl_change_audio_volume(volume);
}

void WidgetMediaControl::slot_get_media_duration(uint32_t duration)
{
    mMediaDuration = duration;
    ui->slider->setRange(0, mMediaDuration * 1000000);

    ui->lbDurationLeft->setText(QString("%1:%2:%3").arg(mMediaDuration / 3600, 2, 10, QLatin1Char('0')).arg(mMediaDuration / 60, 2, 10, QLatin1Char('0')).arg(mMediaDuration % 60, 2, 10, QLatin1Char('0')));
}

void WidgetMediaControl::slot_current_video_frame_time(float time)
{
    uint32_t currentTime = time * 1000000;
    ui->slider->setValue(currentTime);

    ui->lbDuration->setText(QString("%1:%2:%3").arg(uint32_t(time) / 3600, 2, 10, QLatin1Char('0')).arg(uint32_t(time) / 60, 2, 10, QLatin1Char('0')).arg(uint32_t(time) % 60, 2, 10, QLatin1Char('0')));

    uint32_t leftTime = mMediaDuration - time;
    ui->lbDurationLeft->setText(QString("%1:%2:%3").arg(leftTime / 3600, 2, 10, QLatin1Char('0')).arg(leftTime / 60, 2, 10, QLatin1Char('0')).arg(leftTime % 60, 2, 10, QLatin1Char('0')));
}
