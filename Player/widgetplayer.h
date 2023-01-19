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
    enum class PlayerStatus { STATUS_PLAYING  = 1, STATUS_PAUSE, STATUS_CLOSED, STATUS_SEEK };
    explicit WidgetPlayer(QWidget *parent = nullptr);
    ~WidgetPlayer();

private:
    void init();

public:

    void play(const QString &path);

    // 开始播放
    void start();

    // 暂停播放
    void pause();

    // 停止播放
    void closeMedia();

    // 清理播放状态
    void clear();

    // 设置音量
    void setAudioVolume(qreal volume);

    // 视频跳转
    void seek(double position);

    // 截图
    void grapImage();

signals:
    void sgl_thread_update_video_frame();
    void sgl_thread_auto_play_current_media();

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
    void listenMediaPlayStatus();

    uint64_t getCurrentMillisecond();

    void resizeVideo();

    void saveGrabImage();

private slots:
    void slot_thread_auto_play_current_media();

private:
    // 纹理着色器
    QOpenGLShaderProgram mShaderProgram;

    // 纹理对象
    unsigned int mTextureID = -1;

    // 纹理长度
    int mTextureWidth = 0;

    // 纹理高度
    int mTextureHeight = 0;

    // 播放器初始化状态
    bool mPlayerInitializeStatus = true;

    // 视频帧队列
    ThreadSafeQueue<VideoFrame> mQueueVideoFrame;

    // 音频帧队列
    ThreadSafeQueue<AudioFrame> mQueueAudioFrame;

    // 播放状态变量
    std::condition_variable mCvPlayMedia;

    // 解析状态变量
    std::condition_variable mCvParse;

    // 当前视频帧数据
    VideoFrame mCurrentVideoFrame;

    int mAudioSampleSize = 0;
    int mSampleRate = 0;
    int mAudioChannles = 0;

    // 视频跳转位置
    double mSeekDuration = -1;

    // 是否已经播放到索引帧
    bool mArriveTargetFrame = true;

    // 当前需要跳转的位置 （pts）
    int64_t mSeekVideoFrameDuration = -1;
    int64_t mSeekAudioFrameDuration = -1;

    // 多媒体文件开始播放的时间戳
    int64_t mStartMediaTimeStamp = -1;

    // 视频第一帧的时间戳
    int64_t mBeginVideoPTS = -1;

    // 音频第一帧的时间戳
    int64_t mBeginAudioPTS = -1;

    QAudioOutput* mAudioOutput = nullptr;
    qreal mAudioVolume = 0.36;
    QIODevice* mIODevice = nullptr;

    // 解析锁
    std::mutex mMutexParse;

    // 播放锁
    std::mutex mMutexPlayFrame;

    // 视频帧等待锁
    std::mutex mMutexWaitVideoFrame;

    // 音频帧等待锁
    std::mutex mMutexWaitAudioFrame;

    // 视频关闭锁
    std::mutex mMutexCloseMedia;

    // 视频关闭 等待变量
    std::condition_variable mCvCloseMedia;

    // 播放器状态
    PlayerStatus mMediaPlayerStatus = PlayerStatus::STATUS_CLOSED;

//    // 播放状态
//    bool mMediaPauseFlag = false;

    // 当前播放的视频路径
    QString mCurrentMediaPath;

    // 下一个需要播放的视频路径
    QString mNextMediaPath;

    // 解析、播放视频、播放音频状态
    bool mPraseThreadFlag = false;
    bool mPlayVideoThreadFlag = false;
    bool mPlayAudioThreadFlag = false;

    // 是否需要截图
    bool mUserGrapImage = false;
};

#endif // WIDGETPLAYER_H
