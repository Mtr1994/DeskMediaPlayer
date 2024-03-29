﻿#include "widgetplayer.h"
#include "Public/appsignal.h"
#include "Configure/softconfig.h"
#include "Public/ffmpeg.h"

#include <thread>
#include <QAudioFormat>
#include <QAudioOutput>
#include <algorithm>
#include <QStandardPaths>
#include <QDateTime>

// test
#include <QDebug>
#include <QDateTime>

WidgetPlayer::WidgetPlayer(QWidget *parent) : QOpenGLWidget(parent)
{
    init();
}

WidgetPlayer::~WidgetPlayer()
{
    mCvParse.notify_one();
    mCvPlayMedia.notify_all();
}

void WidgetPlayer::init()
{
    mCurrentVideoFrame.linesize = 0;

    connect(this, &WidgetPlayer::sgl_thread_auto_play_current_media, this, &WidgetPlayer::slot_thread_auto_play_current_media, Qt::QueuedConnection);

    // 线程更新视频界面
    connect(this, &WidgetPlayer::sgl_thread_update_video_frame, this, [this] { update();}, Qt::QueuedConnection);

    // 解决锯齿问题
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setSamples(24);
    setFormat(surfaceFormat);
}

void WidgetPlayer::play(const QString &path)
{
    if (!mPlayerInitializeStatus)
    {
        emit AppSignal::getInstance()->sgl_system_output_message("播放器初始化失败");
        return;
    }
    // 正在播放该视频，就不用响应
    if ((mMediaPlayerStatus != PlayerStatus::STATUS_CLOSED) && (mCurrentMediaPath == path)) return;

    if (mMediaPlayerStatus != PlayerStatus::STATUS_CLOSED)
    {
        qDebug() << "------------------------------------------------------------ 中途加入视频要开始了 ------------------------------------------------------------";

        // 记录下一个需要播放的视频路径
        mNextMediaPath = path;

        //mAutoPlayMedia = true;
        mMediaPlayerStatus = PlayerStatus::STATUS_CLOSED;

        // 唤醒 解析线程
        mCvParse.notify_one();

        // 唤醒 播放线程
        mCvPlayMedia.notify_all();
    }
    else
    {
         // 记录当前视频的路径
        mCurrentMediaPath = path;

        if (mCurrentMediaPath.isEmpty()) return;

        qDebug() << "------------------------------------------------------------ 自然加入视频要开始了 ------------------------------------------------------------";

        // 修改播放器状态
        mMediaPlayerStatus = PlayerStatus::STATUS_PLAYING;

        auto funcParse = std::bind(&WidgetPlayer::parse, this, std::placeholders::_1);
        std::thread threadParse(funcParse, path);
        threadParse.detach();
    }
}

void WidgetPlayer::start()
{
    double firstAudioTimeStamp = 0xffffffff;
    if (mQueueAudioFrame.size() > 0)
    {
        AudioFrame frame;
        mQueueAudioFrame.wait_and_pop(frame);
        firstAudioTimeStamp = (frame.pts - mBeginAudioPTS) * frame.timebase * 1000;
    }

    double firstVideoTimeStamp = 0xffffffff;
    if (mQueueVideoFrame.size() > 0)
    {
        VideoFrame frame;
        mQueueVideoFrame.wait_and_pop(frame);
        firstVideoTimeStamp = (frame.pts - mBeginAudioPTS) * frame.timebase * 1000;
    }

    double firstTimeStamp = qMin(firstAudioTimeStamp, firstVideoTimeStamp);

    mStartMediaTimeStamp = getCurrentMillisecond() - firstTimeStamp;

    // 修改播放状态为正在播放
    mMediaPlayerStatus = PlayerStatus::STATUS_PLAYING;

    // 唤醒播放线程，通过该消费者取唤醒生产者
    mCvPlayMedia.notify_all();
}

void WidgetPlayer::pause()
{
    mMediaPlayerStatus = PlayerStatus::STATUS_PAUSE;
}

void WidgetPlayer::closeMedia()
{
    mMediaPlayerStatus = PlayerStatus::STATUS_CLOSED;

    mCvParse.notify_one();

    mCvPlayMedia.notify_all();
}

void WidgetPlayer::clear()
{
    qDebug() << "WidgetPlayer::clear";
    mMediaPlayerStatus = PlayerStatus::STATUS_CLOSED;
    mArriveTargetFrame = true;
    mSeekDuration = -1;
    mSeekVideoFrameDuration = -1;
    mSeekAudioFrameDuration = -1;
    mStartMediaTimeStamp = -1;
    mBeginVideoPTS = -1;
    mBeginAudioPTS = -1;

    mTextureID = -1;
    mTextureWidth = 0;
    mTextureHeight = 0;

    mAudioSampleSize = 0;
    mSampleRate = 0;
    mAudioChannles = 0;

    mUserGrapImage = false;
    mPraseThreadFlag = false;
    mPlayVideoThreadFlag = false;
    mPlayAudioThreadFlag = false;

    // 不能清理这个值
    // mAutoPlayMedia = false;

    if (nullptr != mAudioOutput)
    {
        mAudioOutput->stop();
        delete mAudioOutput;
        mAudioOutput = nullptr;
    }
    mIODevice = nullptr;

    while (!mQueueVideoFrame.empty())
    {
        VideoFrame frame;
        mQueueVideoFrame.wait_and_pop(frame);
        delete [] frame.buffer;
        mQueueVideoFrame.wait_and_pop();
    }

    while (!mQueueAudioFrame.empty())
    {
        AudioFrame frame;
        mQueueAudioFrame.wait_and_pop(frame);
        delete [] frame.data;
        mQueueAudioFrame.wait_and_pop();
    }
}

void WidgetPlayer::setAudioVolume(qreal volume)
{
    mAudioVolume = volume;
    if (nullptr == mAudioOutput) return;
    mAudioOutput->setVolume(mAudioVolume);
}

void WidgetPlayer::seek(double position)
{
    qDebug() << "WidgetPlayer::seek" ;
    // Seek 的时候，不能继续解析，要让播放线程先清理旧的帧
    std::unique_lock<std::mutex> lock(mMutexParse);
    mArriveTargetFrame = false;
    mSeekVideoFrameDuration = 0x0fffffffffffffff;
    mSeekAudioFrameDuration = 0x0fffffffffffffff;

    mMediaPlayerStatus = PlayerStatus::STATUS_SEEK;
    //qDebug() << "seek A" << position;
    mSeekDuration = position;
    mCvPlayMedia.notify_all();
}

void WidgetPlayer::grapImage()
{
    mUserGrapImage = true;
    update();
}

void WidgetPlayer::initializeGL()
{
    // 不调用这句话，就不能使用 gl 开头的函数，程序会崩溃
    initializeOpenGLFunctions();

    // 开启多重采样
    glEnable(GL_MULTISAMPLE);

    // 着色器文件不能使用 UTF-8-BOM 编码，会报错，只能采用 UTF-8 编码

    // 加载顶点着色器
    bool status = mShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Resource/shader/shader.vert");
    if (!status)
    {
        qDebug() << "parse vertex shader fail " << mShaderProgram.log();
        mPlayerInitializeStatus = false;
        return;
    }

    // 加载片段着色器
    status= mShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Resource/shader/shader.frag");
    if (!status)
    {
        qDebug() << "parse fragment shader fail " << mShaderProgram.log();
        mPlayerInitializeStatus = false;
        return;
    }

    // 链接程序
    status = mShaderProgram.link();
    if (!status)
    {
        qDebug() << "link shader fail " << mShaderProgram.log();
        mPlayerInitializeStatus = false;
        return;
    }

    glEnable(GL_MULTISAMPLE);

    glUseProgram(mShaderProgram.programId());

    {
        // 上下翻转 用 1 减去 纹理 坐标 T
        // 左右翻转 用 1 减去 纹理 坐标 S
        float vertices[]
        {
            // ---- 位置 ----       - 纹理坐标 -
             1.0f,  1.0f, 0.0f,    1.0f, 0.0f,   // 左下
             1.0f, -1.0f, 0.0f,    1.0f, 1.0f,   // 左上
            -1.0f, -1.0f, 0.0f,    0.0f, 1.0f,   // 右上
            -1.0f,  1.0f, 0.0f,    0.0f, 0.0f    // 右下
        };

        unsigned int indices[] = { 0, 1, 2, 3 };

        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texture coord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // load and create a texture
        // -------------------------
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &mTextureID);
        glBindTexture(GL_TEXTURE_2D, mTextureID);

        unsigned int texture = glGetUniformLocation(mShaderProgram.programId(), "ourTexture");
        if (texture < 0)
        {
            qDebug() << "can not find uniform ourTexture";
            mPlayerInitializeStatus = false;
            return;
        }
        // 赋值为 0 是因为这里启用的纹理是 GL_TEXTURE0
        glUniform1i(texture, 0);

        // set texture filtering parameters
        // 使用 GL_LINEAR_MIPMAP_LINEAR 的时候，必须调用  glGenerateMipmap 函数
        // 这个多级渐远纹理可以让视频在缩小的时候，不会出现像素化的情况
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // set the texture wrapping parameters
        // set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // 解除绑定，防止其它操作影响到这个纹理
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void WidgetPlayer::paintGL()
{
    glViewport((width() - mTextureWidth) / 2.0, (height() - mTextureHeight) / 2.0, mTextureWidth, mTextureHeight);

    if (mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED)
    {
        glBindTexture(GL_TEXTURE_2D, mTextureID);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else if (mCurrentVideoFrame.linesize <= 0)
    {
        glBindTexture(GL_TEXTURE_2D, mTextureID);
        glGenerateMipmap(GL_TEXTURE_2D);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);
    }
    else
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::unique_lock<std::mutex> lock(mMutexPlayFrame);

        // 写入数据的时候，一定要先绑定纹理单元
        glBindTexture(GL_TEXTURE_2D, mTextureID);

        // 可以让纹理读取数据的时候按照设定的大小读取，这个值概念上跟linesize是一样的，这样就不会把无效的数据读取进去了
        glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentVideoFrame.linesize);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mCurrentVideoFrame.width, mCurrentVideoFrame.height, 0, GL_RGB, GL_UNSIGNED_BYTE, mCurrentVideoFrame.buffer);
        // 数据已经发给显卡，可以删除 CPU 中的数据
        if (mCurrentVideoFrame.buffer)
        {
            delete [] mCurrentVideoFrame.buffer;
            mCurrentVideoFrame.linesize = 0;
        }
        lock.unlock();

        glGenerateMipmap(GL_TEXTURE_2D);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);
    }

    // 截图
    saveGrabImage();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void WidgetPlayer::resizeGL(int w, int h)
{
    Q_UNUSED(w); Q_UNUSED(h);
    resizeVideo();
}

void WidgetPlayer::parse(const QString &path)
{
    qDebug() << "parse media begin ";
    mPraseThreadFlag = true;

    //输入输出封装上下文
    AVFormatContext *formatCtx = nullptr;

    AVCodecContext* codeCtxVideo = nullptr;
    int videoStreamIndex = -1;

    AVCodecContext *codeCtxAudio = nullptr;
    int audioStreamIndex = -1;

    ///查找解码器
    const AVCodec* pCodecVideo = nullptr;
    const AVCodec* pCodecAudio = nullptr;
    int ret = 0;

    //打开文件，解封文件头
    ret = avformat_open_input(&formatCtx, path.toStdString().c_str(), 0, 0);
    if (ret < 0)
    {
        qDebug() << "Could not open input file \n";
        emit AppSignal::getInstance()->sgl_system_output_message("打开媒体文件失败");
        mPraseThreadFlag = false;
        return;
    }

    //获取音频视频流信息 ,h264 flv
    ret = avformat_find_stream_info(formatCtx, 0);
    if (ret < 0)
    {
        qDebug() << "Failed to retrieve input stream information\n";
        mPraseThreadFlag = false;
        return;
    }

    if (formatCtx->nb_streams == 0)
    {
        qDebug() << "Can not find stream\n";
        mPraseThreadFlag = false;
        return;
    }

    int32_t duration = formatCtx->duration;
    double timebase = 1.0 / AV_TIME_BASE;

    // 初始化界面视频时长
    emit AppSignal::getInstance()->sgl_init_media_duration(duration, timebase);

    for (unsigned i = 0; i < formatCtx->nb_streams; i++)
    {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (videoStreamIndex >= 0) break;

            videoStreamIndex = i;
            AVCodecID codecid = formatCtx->streams[i]->codecpar->codec_id;
            if (codecid == AV_CODEC_ID_NONE)
            {
                mPraseThreadFlag = false;
                return;
            }

            ///查找解码器
            pCodecVideo = avcodec_find_decoder(codecid);
            codeCtxVideo = avcodec_alloc_context3(pCodecVideo);
            avcodec_parameters_to_context(codeCtxVideo, formatCtx->streams[i]->codecpar);

            //没有此句会出现：Could not update timestamps for skipped samples
            codeCtxVideo->pkt_timebase = formatCtx->streams[i]->time_base;

            ///打开解码器
            if (avcodec_open2(codeCtxVideo, pCodecVideo, NULL) < 0)
            {
                qDebug() << "Could not open video codec.";
                mPraseThreadFlag = false;
                return;
            }

            // 存在视频流，准备播放
            auto funcPlayVideo= std::bind(&WidgetPlayer::playVideoFrame, this);
            std::thread threadPlayVideo(funcPlayVideo);
            threadPlayVideo.detach();
        }
        else if (formatCtx->streams[i]->codecpar->codec_type== AVMEDIA_TYPE_AUDIO)
        {
            // 默认使用第一个音频源 （可能存在多个音频源，导致开启多个音频播放线程）
            if (audioStreamIndex >= 0) break;

            audioStreamIndex = i;
            AVCodecID codecid = formatCtx->streams[i]->codecpar->codec_id;
            if (codecid == AV_CODEC_ID_NONE) break;

            pCodecAudio = avcodec_find_decoder(formatCtx->streams[i]->codecpar->codec_id);
            codeCtxAudio = avcodec_alloc_context3(pCodecAudio);
            avcodec_parameters_to_context(codeCtxAudio, formatCtx->streams[i]->codecpar);

            //没有此句会出现：Could not update timestamps for skipped samples
            codeCtxAudio->pkt_timebase = formatCtx->streams[i]->time_base;

            ///打开解码器
            if (avcodec_open2(codeCtxAudio, pCodecAudio, NULL) < 0)
            {
                qDebug() << "Could not open audio codec.";
                mPraseThreadFlag = false;
                return;
            }

            mAudioSampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            mSampleRate = codeCtxAudio->sample_rate;
            mAudioChannles = codeCtxAudio->channels;

            // 存在音频流，准备播放
            auto funcPlayAudio= std::bind(&WidgetPlayer::playAudioFrame, this);
            std::thread threadPlayAudio(funcPlayAudio);
            threadPlayAudio.detach();
        }
    }

    // 解析线程启动后，就要配置一个停止线程
    auto funcWait = std::bind(&WidgetPlayer::listenMediaPlayStatus, this);
    std::thread threadWait(funcWait);
    threadWait.detach();

    // 分配一个packet
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    bool seekVideoFlag = false;
    bool seekAudioFlag = false;

    // 解析之前，记录第一帧的 PTS
    mBeginVideoPTS = AV_NOPTS_VALUE;
    mBeginAudioPTS = AV_NOPTS_VALUE;

    // 解析到了哪个帧
    double mLastParseDuration = -1;
    while (mMediaPlayerStatus != PlayerStatus::STATUS_CLOSED)
    {
        std::unique_lock<std::mutex> lock(mMutexParse);
        mCvParse.wait(lock, [this]
        {
            // 除了等待状态，其他状态都要过
            bool flag1 = mMediaPlayerStatus != PlayerStatus::STATUS_PAUSE;

            // 限制队列里面帧的个数，太多了会占内存
            bool flag2 = mQueueVideoFrame.size() < 20;

            // 关闭状态
            bool flag4 = mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED;

            // 跳转状态
            bool flag5 = mMediaPlayerStatus == PlayerStatus::STATUS_SEEK;

            return (flag1 && flag2) || flag4 || flag5;
        });

        // 如果是 停止 状态，直接退出
        if (mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED) break;

        if (mMediaPlayerStatus == PlayerStatus::STATUS_SEEK)
        {
            // 清空已经读取的帧数据
            if (nullptr != codeCtxVideo) avcodec_flush_buffers(codeCtxVideo);
            if (nullptr != codeCtxAudio) avcodec_flush_buffers(codeCtxAudio);
            avformat_flush(formatCtx);

            mStartMediaTimeStamp = -1;

            double position = mSeekDuration + mBeginVideoPTS * mCurrentVideoFrame.timebase;

            // 823718880
            //qDebug() << "position " << position << " " << mBeginVideoPTS << " " << mCurrentVideoFrame.timebase << " " << AV_TIME_BASE << " " << mLastParseDuration;
            //qDebug() << "timestamp " << QString::number(position * AV_TIME_BASE, 'f', 2);

            int flags = position > mLastParseDuration ? AVSEEK_FLAG_FRAME : AVSEEK_FLAG_BACKWARD;
            int ret = av_seek_frame(formatCtx, -1, position * AV_TIME_BASE, flags);

            if (ret >= 0)
            {
                mSeekDuration = -1;
                seekVideoFlag = true;
                seekAudioFlag = true;
            }
            else
            {
                mArriveTargetFrame = true;
                mSeekDuration = -1;
                qDebug() << "seek video failed";
                mMediaPlayerStatus = PlayerStatus::STATUS_CLOSED;
                continue;
            }

            mMediaPlayerStatus = PlayerStatus::STATUS_PLAYING;
        }

        ret = av_read_frame(formatCtx, packet);
        if (ret < 0)
        {
            qDebug() << "can not find any frame";
            break;
        }

        if (packet->stream_index == videoStreamIndex)
        {
            ret = avcodec_send_packet(codeCtxVideo, packet);
            if (ret < 0)
            {
                av_packet_unref(packet);
                continue;
            }

            SwsContext* pSwsCtx = nullptr;
            while (ret >= 0)
            {
                ret = avcodec_receive_frame(codeCtxVideo, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    av_frame_unref(frame);
                    break;
                }
                else if (ret < 0)
                {
                    av_frame_unref(frame);
                    break;
                }

                frame->pts = frame->best_effort_timestamp;

                if (frame->pts == AV_NOPTS_VALUE) frame->pts = frame->pkt_dts = 1;
                if (mBeginVideoPTS == AV_NOPTS_VALUE) mBeginVideoPTS = frame->pts;

                // 记录跳转位置
                if (seekVideoFlag)
                {
                    mSeekVideoFrameDuration = frame->pts;
                    seekVideoFlag = false;
                    qDebug() << "seekVideoFlag " << frame->pts;
                }

                if (mStartMediaTimeStamp < 0)
                {
                    double timebase = (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;
                    mStartMediaTimeStamp = getCurrentMillisecond() - (frame->pts - mBeginVideoPTS) * timebase * 1000;

                    //qDebug() << "mStartMediaTimeStamp video " << mStartMediaTimeStamp;
                }

                if (nullptr == pSwsCtx)
                {
                    pSwsCtx = sws_getContext(codeCtxVideo->width, codeCtxVideo->height, (AVPixelFormat)frame->format, codeCtxVideo->width, codeCtxVideo->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
                }

                int pixWidth = frame->width;
                int pixHeight = frame->height;
                int linesize0 = frame->linesize[0];

                uint8_t *buffer = new uint8_t[linesize0 * pixHeight * 3];
                memset(buffer, 0, linesize0 * pixHeight * 3);

                VideoFrame videoFrame = { frame->pts, pixWidth, pixHeight, mAudioSampleSize, 0, buffer, 0 };
                if (frame->format == AV_PIX_FMT_RGB24)
                {
                    memcpy(buffer, frame->data[0], linesize0 * pixHeight * 3);
                    videoFrame.timebase = (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;
                }
                else
                {
                    uint8_t *data[AV_NUM_DATA_POINTERS] = { buffer };
                    int linesize[AV_NUM_DATA_POINTERS] = { linesize0 * 3, 0, 0, 0, 0, 0, 0, 0 };
                    int ret = sws_scale(pSwsCtx, (uint8_t const * const *) frame->data, frame->linesize, 0, frame->height, data, linesize);
                    if (ret <= 0)  memset(buffer, 0, linesize0 * pixHeight * 3);
                    videoFrame.timebase = (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;
                }

                mLastParseDuration = (frame->pts - mBeginVideoPTS) * videoFrame.timebase;

                videoFrame.linesize = linesize0;
                mQueueVideoFrame.push(videoFrame);

                // 通知播放
                mCvPlayMedia.notify_all();

                av_frame_unref(frame);
            }
            av_packet_unref(packet);
            sws_freeContext(pSwsCtx);
        }
        else if (packet->stream_index == audioStreamIndex)
        {
            ret = avcodec_send_packet(codeCtxAudio, packet);
            if (ret < 0)
            {
                av_packet_unref(packet);
                continue;
            }

            SwrContext* swrCtx = nullptr;
            while (ret >= 0)
            {
                ret = avcodec_receive_frame(codeCtxAudio, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    av_frame_unref(frame);
                    break;
                }
                else if (ret < 0)
                {
                    av_frame_unref(frame);
                    break;
                }

                frame->pts = frame->best_effort_timestamp;

                if (mBeginAudioPTS == AV_NOPTS_VALUE) mBeginAudioPTS = frame->pts;

                // 记录跳转位置
                if (seekAudioFlag)
                {
                    mSeekAudioFrameDuration = frame->pts;
                    seekAudioFlag = false;
                    //qDebug() << "seekAudioFlag " << frame->pts;
                }

               // qDebug() << "mStartMediaTimeStamp audio0 " << mStartMediaTimeStamp;
                if (mStartMediaTimeStamp < 0)
                {
                    double timebase = (double)codeCtxAudio->pkt_timebase.num / codeCtxAudio->pkt_timebase.den;
                    mStartMediaTimeStamp = getCurrentMillisecond() - (frame->pts - mBeginAudioPTS) * timebase * 1000;
                }

                swrCtx = swr_alloc_set_opts(nullptr, frame->channel_layout, AV_SAMPLE_FMT_S16, codeCtxAudio->sample_rate, codeCtxAudio->channel_layout, codeCtxAudio->sample_fmt, codeCtxAudio->sample_rate, 0, nullptr);
                swr_init(swrCtx);
                int bufsize = av_samples_get_buffer_size(nullptr, frame->channels, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
                uint8_t *buf = new uint8_t[bufsize];
                int tmpSize = swr_convert(swrCtx, &buf, frame->nb_samples, (const uint8_t**)(frame->data), frame->nb_samples);
                if (tmpSize <= 0)
                {
                    delete [] buf;
                    av_frame_unref(frame);
                    break;
                }

                AudioFrame audio = { frame->pts, bufsize, 0, buf, frame->nb_samples};
                audio.timebase = (double)codeCtxAudio->pkt_timebase.num / codeCtxAudio->pkt_timebase.den;
                mQueueAudioFrame.push(audio);

                mCvPlayMedia.notify_all();
                av_frame_unref(frame);
            }

            av_packet_unref(packet);
            swr_free(&swrCtx);
        }
        else
        {
            ret = avcodec_send_packet(codeCtxAudio, packet);
            if (ret < 0)
            {
                av_packet_unref(packet);
                continue;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(codeCtxAudio, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    av_frame_unref(frame);
                    break;
                }
                else if (ret < 0)
                {
                    av_frame_unref(frame);
                    break;
                }

                av_frame_unref(frame);
            }

            av_packet_unref(packet);
        }
    }

    av_frame_free(&frame);
    av_packet_unref(packet);
    av_packet_free(&packet);
    avcodec_free_context(&codeCtxVideo);
    avcodec_free_context(&codeCtxAudio);

    mPraseThreadFlag = false;

    // 通知播放线程
    mCvPlayMedia.notify_all();

    qDebug() << "parse media over ";

    // 通知关闭等待线程
    std::unique_lock<std::mutex> lock(mMutexCloseMedia);
    mCvCloseMedia.notify_one();
}

void WidgetPlayer::playVideoFrame()
{
    qDebug() << "play video begin ";
    mPlayVideoThreadFlag = true;

    std::unique_lock<std::mutex> lockNextFrame(mMutexWaitVideoFrame);
    while (mMediaPlayerStatus != PlayerStatus::STATUS_CLOSED)
    {
        // 解析结束且视频帧为空的时候，关闭播放线程
        if (!mPraseThreadFlag && mQueueVideoFrame.empty()) break;

        // 通知准备解析下一帧数据帧
        if (mPraseThreadFlag) mCvParse.notify_one();

        // 防止循环空转
        mCvPlayMedia.wait(lockNextFrame, [this]
        {
            // 不是 暂停 状态
            bool flag1 = mMediaPlayerStatus != PlayerStatus::STATUS_PAUSE;

            // 视频队列非空
            bool flag2 = !mQueueVideoFrame.empty();

            // 处于 关闭 状态
            bool flag3 = mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED;

            // 处于 跳转 状态
            bool flag4 = mMediaPlayerStatus == PlayerStatus::STATUS_SEEK;

            // 解析完且没有数据帧
            bool flag5 = !mPraseThreadFlag && mQueueVideoFrame.empty();

         //   qDebug() << "video falgs " << flag1 << " " << flag2 << " " << flag3;

            return (flag1 && flag2) || flag3 || flag4 || flag5;
        });

        if (mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED) break;
        if (!mPraseThreadFlag && mQueueVideoFrame.empty()) break;

        VideoFrame frame;
        mQueueVideoFrame.wait_and_pop(frame);

       // qDebug() << "frame " << frame.pts << " " << mSeekVideoFrameDuration;;

        int64_t currentTimeStamp = getCurrentMillisecond();
        int64_t time = (frame.pts - mBeginVideoPTS) * frame.timebase * 1000;
        //if ((mStartMediaTimeStamp < 0) && (frame.pts > 0 && (mSeekVideoFrameDuration < 0))) mStartMediaTimeStamp = currentTimeStamp - time;

        double diff = currentTimeStamp - mStartMediaTimeStamp - time;

        //qDebug() << "video diff " << diff << " " << mSeekVideoFrameDuration;

        if ((mSeekVideoFrameDuration < 0) && (diff < 0))
        {
            int32_t milliseconds = diff * -1;
            //qDebug() << "video wait " << milliseconds;
            mCvPlayMedia.wait_for(lockNextFrame, std::chrono::milliseconds(milliseconds));
            continue;
        }

        mQueueVideoFrame.wait_and_pop();

        std::unique_lock<std::mutex> lock(mMutexPlayFrame);
        if (mCurrentVideoFrame.linesize > 0) delete [] mCurrentVideoFrame.buffer;
        mCurrentVideoFrame = frame;
        resizeVideo();
        lock.unlock();

        emit sgl_thread_update_video_frame();

        if (mSeekVideoFrameDuration == frame.pts)
        {
            mSeekVideoFrameDuration = -1;
          //  mStartMediaTimeStamp = -1;
            mArriveTargetFrame = true;
        }

        // 更新视频当前播放时长（frame . pts 不一定从 0 开始）
        if ((mSeekVideoFrameDuration < 0) && mArriveTargetFrame)
        {
            emit AppSignal::getInstance()->sgl_thread_current_video_frame_time(frame.pts - mBeginVideoPTS, frame.timebase);
        }
    }

    mPlayVideoThreadFlag = false;

    qDebug() << "play video over ";

    // 通知关闭等待线程
    std::unique_lock<std::mutex> lock(mMutexCloseMedia);
    mCvCloseMedia.notify_one();
}

void WidgetPlayer::playAudioFrame()
{
    qDebug() << "play audio begin ";
    mPlayAudioThreadFlag = true;
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(mSampleRate);
    audioFormat.setChannelCount(mAudioChannles);
    audioFormat.setSampleSize(8 * mAudioSampleSize);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setCodec("audio/pcm");

    if (nullptr == mAudioOutput)
    {
        mAudioOutput = new QAudioOutput(audioFormat);
        mAudioVolume = SoftConfig::getInstance()->getValue("Volume", "value").toUInt() * 1.0 / 100;
        mAudioOutput->setVolume(mAudioVolume);
    }

    mAudioOutput->setBufferSize(mSampleRate * mAudioChannles * mAudioSampleSize);
    mIODevice = mAudioOutput->start();

    std::unique_lock<std::mutex> lockNextFrame(mMutexWaitAudioFrame);

    // 提取音频帧
    while (mMediaPlayerStatus != PlayerStatus::STATUS_CLOSED)
    {
        // 解析结束且视频帧为空的时候，关闭播放线程
        if (!mPraseThreadFlag && mQueueAudioFrame.empty()) break;

        // 通知准备解析下一帧数据帧
        if (mPraseThreadFlag) mCvParse.notify_one();

        mCvPlayMedia.wait(lockNextFrame, [this]
        {
            // 不是 暂停 状态
            bool flag1 = mMediaPlayerStatus != PlayerStatus::STATUS_PAUSE;

            // 音频队列非空
            bool flag2 = !mQueueAudioFrame.empty();

            // 处于 关闭 状态
            bool flag3 = mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED;

            // 处于 跳转 状态
            bool flag4 = mMediaPlayerStatus == PlayerStatus::STATUS_SEEK;

            // 解析完且没有数据帧
            bool flag5 = !mPraseThreadFlag && mQueueAudioFrame.empty();

            //qDebug() << "audio wait " << (flag1 && flag2) << " " << flag3 << " " << flag4 << " " << flag5;
            return (flag1 && flag2) || flag3 || flag4 || flag5;
        });

        if (mMediaPlayerStatus == PlayerStatus::STATUS_CLOSED) break;
        if (!mPraseThreadFlag && mQueueAudioFrame.empty()) break;

        AudioFrame frame;
        mQueueAudioFrame.wait_and_pop(frame);

        int64_t currentTimeStamp = getCurrentMillisecond();
        int64_t time = (frame.pts - mBeginAudioPTS) * frame.timebase * 1000;
        if (frame.pts == AV_NOPTS_VALUE) time = 0;
        //if ((mStartMediaTimeStamp < 0) && (frame.pts != AV_NOPTS_VALUE)) mStartMediaTimeStamp = currentTimeStamp - time;

        int64_t diff = currentTimeStamp - mStartMediaTimeStamp - time;
        // 判断是否可以播放该帧了(也判断是否需要跳过)
        if ((mSeekAudioFrameDuration < 0) && (diff < 0))
        {
            int32_t milliseconds = diff * -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            continue;
        }
        mQueueAudioFrame.wait_and_pop();

        if (mSeekAudioFrameDuration == frame.pts)
        {
            mSeekAudioFrameDuration = -1;
        }

        if (nullptr == frame.data) continue;
        if (nullptr != mIODevice && mAudioVolume > 0)
        {
            if ((mMediaPlayerStatus != PlayerStatus::STATUS_PAUSE) && mSeekAudioFrameDuration < 0) mIODevice->write((const char*)frame.data, frame.size);
            delete frame.data;
        }

        // 如果 PTS 异常，就根据音频播放时间，强制休眠一点时间
        if (frame.pts == AV_NOPTS_VALUE)
        {
            uint64_t milliseconds = 1000.0 * frame.samples / mAudioChannles / mSampleRate;
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        }
    }

    mPlayAudioThreadFlag = false;

    qDebug() << "play audio over ";

    // 通知关闭等待线程
    std::unique_lock<std::mutex> lock(mMutexCloseMedia);
    mCvCloseMedia.notify_one();
}

void WidgetPlayer::listenMediaPlayStatus()
{
    std::unique_lock<std::mutex> lock(mMutexCloseMedia);
    while (true)
    {
        mCvCloseMedia.wait(lock, [this]
        {
            qDebug() << "try close media " << mPraseThreadFlag << " " << mPlayVideoThreadFlag << " " << mPlayAudioThreadFlag;
            return !mPraseThreadFlag && !mPlayVideoThreadFlag && !mPlayAudioThreadFlag;
        });
        break;
    }

    // 清理最后一帧画面
    emit sgl_thread_update_video_frame();

    // 彻底停止解析 播放视频 播放音频 后再清理状态
    clear();

    qDebug() << "------------------------------------------------------------ 旧的视频被关闭了 ------------------------------------------------------------";

    if (mNextMediaPath.isEmpty())
    {
        // 发送视频结束信号
        emit AppSignal::getInstance()->sgl_thread_finish_play_video();
        return;
    }
    emit sgl_thread_auto_play_current_media();
}

uint64_t WidgetPlayer::getCurrentMillisecond()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void WidgetPlayer::resizeVideo()
{
    int widgetWidth = width();
    int widgetHeight = height();
    if(widgetHeight <= 0) widgetHeight = 1;

    if (mCurrentVideoFrame.width <= 0 || mCurrentVideoFrame.height <= 0)
    {
        mTextureWidth = 0;
        mTextureHeight = 0;
        return;
    }

    double rate = (double)mCurrentVideoFrame.height / mCurrentVideoFrame.width;
    double widthSize = widgetWidth;
    double heightSize = widgetWidth * rate;

    if (widgetHeight < heightSize)
    {
        heightSize = widgetHeight;
        widthSize = heightSize / rate;
    }

    mTextureWidth = widthSize;
    mTextureHeight = heightSize;
}

// 函数只能在 paintGL 中调用
void WidgetPlayer::saveGrabImage()
{
    // 渲染完再截图
    if (!mUserGrapImage) return;

    mUserGrapImage = false;

    uint32_t textureWidth = mTextureWidth;
    uint32_t textureHeight = mTextureHeight;
    // opengl 默认读取方式是 4 字节对齐方式，如果宽度不够 4 字节，函数还是会读四字节, 就会导致直接乘积得到的内存不够，删除的时候就会越界崩溃
    uint32_t len = (textureWidth + (4 - textureWidth % 4)) * textureHeight * 3;
    uint8_t *buffer = new uint8_t[len];
    if (nullptr == buffer) return;
    memset(buffer, 0, len);

    // 设定读取的内存对其方式 （只能为 1 2 4 8）
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    int x = (width() - textureWidth) / 2.0;
    int y = (height() - textureHeight) / 2.0;

    // 这里的 FBO 为什么是 2 号，我不是很懂啊
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 2);
    glReadPixels(x, y, textureWidth, textureHeight, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    auto func = [buffer, textureWidth, textureHeight]
    {
        QImage image(buffer, textureWidth, textureHeight, QImage::Format_RGB888);
        image = image.mirrored(false, true);

        // 按照视频的长宽比设置固定大小的缩放
//        QPixmap pixmap = QPixmap::fromImage(image);
//        pixmap = pixmap.scaled(QSize(1920, 1080), Qt::KeepAspectRatio);

        QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QString path = QString("%1/Capture_%2.png").arg(desktop, QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"));
        //bool status = pixmap.save(path, "png");
        bool status = image.save(path, "png");
        emit AppSignal::getInstance()->sgl_thread_save_capture_status(status, path);
        delete [] buffer;
    };
    std::thread th(func);
    th.detach();
}

void WidgetPlayer::slot_thread_auto_play_current_media()
{
    play(mNextMediaPath);
    mNextMediaPath.clear();
}
