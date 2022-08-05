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

    void slot_get_media_duration(uint32_t duration);

    void slot_current_video_frame_time(float time);

private:
    Ui::WidgetMediaControl *ui;

    // 视频时长
    uint32_t mMediaDuration = 0;
};

#endif // WIDGETMEDIACONTROL_H
