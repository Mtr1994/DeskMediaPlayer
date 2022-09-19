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

private slots:
    void slot_btn_play_click();

    void slot_volume_value_change(int volume);

    void slot_init_media_duration(int64_t duration, double timebase);

    void slot_thread_current_video_frame_time(int64_t pts, float timebase);

private:
    Ui::WidgetMediaControl *ui;

    // 视频时长
    int64_t mMediaBaseDuration = 0;

    //
    double mMediaTimeBase = 0.0;
};

#endif // WIDGETMEDIACONTROL_H
