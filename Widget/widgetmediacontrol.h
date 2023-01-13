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

    void init();

    void changePlayStatus();

    void addMediaPath(const QString &path);

private slots:
    void slot_btn_play_previous_frame();

    void slot_btn_play_click();

    void slot_btn_play_next_frame();

    void slot_volume_value_change(int volume);

    void slot_init_media_duration(int64_t duration, double timebase);

    void slot_thread_current_video_frame_time(int64_t pts, float timebase);

    void slot_thread_finish_play_video();

    void slot_btn_media_list_click();

    void slot_start_play_target_media(const QString &path);

private:
    Ui::WidgetMediaControl *ui;

    // 视频时长
    int64_t mMediaBaseDuration = 0;

    //
    double mMediaTimeBase = 0.0;

    // 播放过的视频路径
    QStringList mListMediaPath;

    // 当前播放地址
    QString mCurrentMediaPath;

    // 是否按下滚动条
    bool mSliderPressed = false;
};

#endif // WIDGETMEDIACONTROL_H
