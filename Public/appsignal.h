﻿#ifndef APPSIGNAL_H
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

};

#endif // APPSIGNAL_H