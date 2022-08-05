#ifndef WIDGETPLAYER_H
#define WIDGETPLAYER_H

#include "Common/common.h"
#include "Public/threadsafequeue.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <mutex>

class QAudioOutput;
class QIODevice;
class WidgetPlayer : public QOpenGLWidget, protected QOpenGLFunctions
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

    // 停止播放
    void stop();

    // 设置音量
    void setAudioVolume(qreal volume);

signals:
    void sgl_thead_update_video_frame();

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

    uint64_t getCurrentMillisecond();

private:
    QOpenGLShaderProgram mShaderProgram;

    // VBO
    QOpenGLBuffer mVertexBufferObject;

    // 纹理层
    unsigned int mTextureArray[3] = { 0 };

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

    // 视频展示尺寸
    int mVideoWidth;
    int mVideoHeight;

    int mSampleSize = 0;
    int mSampleRate = 0;
    int mAudioChannles = 0;

    // 视频开始播放的时间戳
    uint64_t mStartTimeStamp = 0;;

    QAudioOutput* mAudioOutput = nullptr;
    qreal mAudioVolume = 0.36;
    QIODevice* mIODevice = nullptr;

    // 播放锁
    std::mutex mMutexPlayFrame;

    // 播放器状态
    bool mMediaPlayFlag = false;

    // 播放状态
    bool mMediaPauseFlag = false;
};

#endif // WIDGETPLAYER_H
