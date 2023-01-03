#ifndef WIDGETPLAYER_H
#define WIDGETPLAYER_H

#include "Common/common.h"
#include "Public/threadsafequeue.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <mutex>

class QAudioOutput;
class QIODevice;
class WidgetPlayer : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit WidgetPlayer(QWidget *parent = nullptr);
    ~WidgetPlayer();

    void play(const QString &path);

    // 开始播放
    void start();

    // 暂停播放
    void pause();

    // 清理播放状态
    void clear();

    // 设置音量
    void setAudioVolume(qreal volume);

    // 视频跳转
    void seek(int position);

signals:
    void sgl_thread_update_video_frame();
    void sgl_thread_media_play_stop();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    // 视频解析线程
    void parse(const QString &path);

    // 视频播放线程
    void playVideoFrame();

    // 音频播放线程
    void playAudioFrame();

    // 等待当前播放线程结束
    void waitMediaPlayStop();

    uint64_t getCurrentMillisecond();

    void resizeVideo();

private slots:
    void slot_thread_media_play_stop();

private:
    QOpenGLShaderProgram mShaderProgram;

    // 纹理对象
    unsigned int mTextureID = -1;

    // 纹理长度
    int mTextureWidth = 0;

    // 纹理高度
    int mTextureHeight = 0;

    // 视频帧队列
    ThreadSafeQueue<VideoFrame> mQueueVideoFrame;

    // 音频帧队列
    ThreadSafeQueue<AudioFrame> mQueueAudioFrame;

    // 播放状态变量
    std::condition_variable mCvPlayMedia;

    // 当前视频帧数据
    VideoFrame mCurrentVideoFrame;

    // 当前音频帧
    AudioFrame mCurrentAudioFrame;

    int mSampleSize = 0;
    int mSampleRate = 0;
    int mAudioChannles = 0;
    int mAudioSampleFormat = 0;

    // 视频跳转位置
    int64_t mSeekDuration = -1;

    // 是否已经播放到索引帧
    bool mArriveTargetFrame = true;

    // 当前需要跳转的位置 （pts）
    int64_t mSeekVideoFrameDuration = -1;
    int64_t mSeekAudioFrameDuration = -1;

    // 视频开始播放的时间戳
    int64_t mStartTimeStamp = 0;

    // 视频第一帧的时间戳
    int64_t mBeginVideoTimeStamp = -1;

    // 音频第一帧的时间戳
    int64_t mBeginAudioTimeStamp = -1;

    QAudioOutput* mAudioOutput = nullptr;
    qreal mAudioVolume = 0.36;
    QIODevice* mIODevice = nullptr;

    // 播放锁
    std::mutex mMutexPlayFrame;

    // 播放器状态
    bool mMediaPlayFlag = false;

    // 播放状态
    bool mMediaPauseFlag = false;

    // 当前视频路径
    QString mMediaPath;

    // 解析、播放视频、播放音频状态
    volatile bool mPraseThreadFlag = false;
    volatile bool mPlayVideoThreadFlag = false;
    volatile bool mPlayAudioThreadFlag = false;
};

#endif // WIDGETPLAYER_H
