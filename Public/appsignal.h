#ifndef APPSIGNAL_H
#define APPSIGNAL_H

#include <QObject>

class AppSignal : public QObject
{
    Q_OBJECT
private:
    explicit AppSignal(QObject *parent = nullptr);
    AppSignal(const AppSignal& signal) = delete;
    AppSignal& operator=(const AppSignal& signal) = delete;

public:
    static AppSignal* getInstance();

signals:
    // 开始播放
    void sgl_start_play_video();

    // 暂停播放
    void sgl_pause_play_video();

    // 音量改变
    void sgl_change_audio_volume(int volume);

    // 视频时长 ( duration 是用于计算时长和跳转，timebase 是用于计算时长的，此处需要分开)
    void sgl_init_media_duration(int64_t duration, double timebase);

    // 视频当前播放帧
    void sgl_thread_current_video_frame_time(int64_t pts, float timebase);

    // 视频播放位置跳转
    void sgl_seek_video_position(int position);

    // 播放结束信号 (通过视频帧不一定能准确判断结束)
    void sgl_thread_finish_play_video();

    // 全屏播放
    void sgl_media_show_full_screen();

    // 截图保存完成
    void sgl_thread_save_capture_status(bool status, const QString &path);

    // 全局消息
    void sgl_system_output_message(const QString &message);

    // 开始播放视频
    void sgl_start_play_target_media(const QString &path);
};

#endif // APPSIGNAL_H
