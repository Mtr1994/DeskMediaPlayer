#include "widgetplayer.h"
#include "Public/appsignal.h"
#include "Configure/softconfig.h"

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/opt.h>
    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
    #include "libavutil/opt.h"
}

#include <thread>
#include <QAudioFormat>
#include <QAudioOutput>
#include <algorithm>

// test
#include <QDebug>

WidgetPlayer::WidgetPlayer(QWidget *parent) : QOpenGLWidget(parent)
{
    mCurrentVideoFrame.pts = -1;
    mCurrentAudioFrame.pts = -1;
}

WidgetPlayer::~WidgetPlayer()
{
    //glDeleteBuffers();
    //glDeleteProgram();

    // 立即停止播放
    stop();

    if (nullptr != mAudioOutput) delete mAudioOutput;
}

void WidgetPlayer::play(const QString &path)
{
    if (path.isEmpty()) return;

    // 线程更新视频界面
    connect(this, &WidgetPlayer::sgl_thead_update_video_frame, this, [this] { update();}, Qt::QueuedConnection);

    mMediaPlayFlag = true;

    auto funcParse = std::bind(&WidgetPlayer::parse, this, std::placeholders::_1);
    std::thread threadParse(funcParse, path);
    threadParse.detach();

    // 开始播放
    start();
}

void WidgetPlayer::start()
{
    mMediaPauseFlag = false;
    mStartTimeStamp = 0;
}

void WidgetPlayer::pause()
{
    mMediaPauseFlag = true;
}

void WidgetPlayer::stop()
{
    mMediaPlayFlag = false;

    while (!mQueueVideoFrame.empty())
    {
        VideoFrame frame;
        mQueueVideoFrame.wait_and_pop(frame);
        delete [] frame.d1;
        delete [] frame.d2;
        delete [] frame.d3;
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
    mAudioOutput->setVolume(mAudioVolume);
}

void WidgetPlayer::initializeGL()
{
    initializeOpenGLFunctions();

    mShaderProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/Resourse/shader/shader.vert");
    mShaderProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/Resourse/shader/shader.frag");
    mShaderProgram.link();

    GLfloat points[]
    {
        -1.0f, 1.0f,
         1.0f, 1.0f,
         1.0f, -1.0f,
        -1.0f, -1.0f,

        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
        0.0f,1.0f
    };

    mVertexBufferObject.create();
    mVertexBufferObject.bind();
    mVertexBufferObject.allocate(points, sizeof(points));

    glGenTextures(3, mTextureArray);
}

void WidgetPlayer::paintGL()
{
    glViewport((this->width() - mVideoWidth) / 2.0, (this->height() - mVideoHeight) / 2.0, mVideoWidth, mVideoHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (mCurrentVideoFrame.pts < 0) return;

    mShaderProgram.bind();
    mVertexBufferObject.bind();
    mShaderProgram.enableAttributeArray("vertexIn");
    mShaderProgram.enableAttributeArray("textureIn");
    mShaderProgram.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    mShaderProgram.setAttributeBuffer("textureIn", GL_FLOAT, 2 * 4 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    std::unique_lock<std::mutex> lock(mMutexPlayFrame);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureArray[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentVideoFrame.linesize1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mCurrentVideoFrame.width1, mCurrentVideoFrame.height1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mCurrentVideoFrame.d1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureArray[1]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentVideoFrame.linesize2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mCurrentVideoFrame.width2, mCurrentVideoFrame.height2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mCurrentVideoFrame.d2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTextureArray[2]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentVideoFrame.linesize3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mCurrentVideoFrame.width3, mCurrentVideoFrame.height3, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mCurrentVideoFrame.d3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    lock.unlock();

    mShaderProgram.setUniformValue("textureY", 0);
    mShaderProgram.setUniformValue("textureU", 1);
    mShaderProgram.setUniformValue("textureV", 2);
    glDrawArrays(GL_QUADS, 0, 4);

    mShaderProgram.disableAttributeArray("vertexIn");
    mShaderProgram.disableAttributeArray("textureIn");
    mShaderProgram.release();
}

void WidgetPlayer::resizeGL(int w, int h)
{
    if(h <= 0) h = 1;

    if (mCurrentVideoFrame.pts < 0)
    {
        mVideoWidth = 0;
        mVideoHeight = 0;
        return;
    }

    double rate = (double)mCurrentVideoFrame.height / mCurrentVideoFrame.width;
    double widthSize = w;
    double heightSize = w * rate;

    if (h < heightSize)
    {
        heightSize = h;
        widthSize = heightSize / rate;
    }

    mVideoWidth = widthSize;
    mVideoHeight = heightSize;
}

void WidgetPlayer::parse(const QString &path)
{
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
        printf("Could not open input file \n");
        return;
    }

    //获取音频视频流信息 ,h264 flv
    ret = avformat_find_stream_info(formatCtx, 0);
    if (ret < 0)
    {
        printf("Failed to retrieve input stream information\n");
        return;
    }

    if (formatCtx->nb_streams == 0)
    {
        printf("can not find stream\n");
        return;
    }

    for (unsigned i = 0; i < formatCtx->nb_streams; i++)
    {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            AVCodecID codecid = formatCtx->streams[i]->codecpar->codec_id;
            if (codecid == AV_CODEC_ID_NONE) return;

            ///查找解码器
            pCodecVideo = avcodec_find_decoder(codecid);
            codeCtxVideo = avcodec_alloc_context3(pCodecVideo);
            avcodec_parameters_to_context(codeCtxVideo, formatCtx->streams[i]->codecpar);

            //没有此句会出现：Could not update timestamps for skipped samples
            codeCtxVideo->pkt_timebase = formatCtx->streams[i]->time_base;

            ///打开解码器
            if (avcodec_open2(codeCtxVideo, pCodecVideo, NULL) < 0)
            {
                printf("Could not open codec.");
                return;
            }

            // 存在视频流，准备播放
            auto funcPlayVideo= std::bind(&WidgetPlayer::playVideoFrame, this);
            std::thread threadPlayVideo(funcPlayVideo);
            threadPlayVideo.detach();
        }
        else if (formatCtx->streams[i]->codecpar->codec_type== AVMEDIA_TYPE_AUDIO)
        {
            audioStreamIndex = i;
            AVCodecID codecid = formatCtx->streams[i]->codecpar->codec_id;
            if (codecid == AV_CODEC_ID_NONE) return;

            pCodecAudio = avcodec_find_decoder(formatCtx->streams[i]->codecpar->codec_id);
            codeCtxAudio = avcodec_alloc_context3(pCodecAudio);
            avcodec_parameters_to_context(codeCtxAudio, formatCtx->streams[i]->codecpar);

            //没有此句会出现：Could not update timestamps for skipped samples
            codeCtxAudio->pkt_timebase = formatCtx->streams[i]->time_base;

            ///打开解码器
            if (avcodec_open2(codeCtxAudio, pCodecAudio, NULL) < 0) {
                printf("Could not open codec.");
                return;
            }

            mSampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
            mSampleRate = codeCtxAudio->sample_rate;
            mAudioChannles = codeCtxAudio->channels;
            mAudioSampleFormat = codeCtxAudio->sample_fmt;

            // 存在音频流，准备播放
            auto funcPlayAudio= std::bind(&WidgetPlayer::playAudioFrame, this);
            std::thread threadPlayAudio(funcPlayAudio);
            threadPlayAudio.detach();
        }
    }

    int32_t duration = formatCtx->duration * 1.0 / AV_TIME_BASE;

    // 视频时长
    emit AppSignal::getInstance()->sgl_get_media_duration(duration);

    auto sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
    // 分配一个packet
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int milliseconds = 0;

    std::mutex mutexParse;
    std::condition_variable cvParse;

    while (mMediaPlayFlag)
    {
        std::unique_lock<std::mutex> lock(mutexParse);
        if(cvParse.wait_for(lock, std::chrono::milliseconds(milliseconds)) == std::cv_status::timeout)
        {
            ret = av_read_frame(formatCtx, packet);
            if (ret < 0) break;
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
                    if (mBeginTimeStamp < 0) mBeginTimeStamp = frame->pts;

                    if (nullptr == pSwsCtx)
                    {
                        pSwsCtx = sws_getContext(codeCtxVideo->width, codeCtxVideo->height, (AVPixelFormat)frame->format, codeCtxVideo->width, codeCtxVideo->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
                    }

                    int pixWidth = frame->width;
                    int pixHeight = frame->height;

                    uint8_t *d1 = new uint8_t[frame->linesize[0] * pixHeight];
                    uint8_t *d2 = new uint8_t[frame->linesize[0] * pixHeight / 4];
                    uint8_t *d3 = new uint8_t[frame->linesize[0] * pixHeight / 4];

                    VideoFrame videoFrame = { frame->pts - mBeginTimeStamp, pixWidth, pixHeight, sampleSize, 0, d1, 0, 0, 0, d2, 0, 0, 0, d3, 0, 0, 0 };
                    if (frame->format == AV_PIX_FMT_YUV420P)
                    {
                        memcpy(d1, frame->data[0], frame->linesize[0] * pixHeight);
                        memcpy(d2, frame->data[1], frame->linesize[0] * pixHeight / 4);
                        memcpy(d3, frame->data[2], frame->linesize[0] * pixHeight / 4);

                        videoFrame.timebase = (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;
                    }
                    else
                    {
                        uint8_t *data[AV_NUM_DATA_POINTERS] = { (uint8_t *)d1, (uint8_t *)d2, (uint8_t *)d3 };
                        int ret = sws_scale(pSwsCtx, (uint8_t const * const *) frame->data, frame->linesize, 0, frame->height, data, frame->linesize);
                        if (ret <= 0)
                        {
                            memset(d1, 0, frame->linesize[0] * pixHeight);
                            memset(d2, 0, frame->linesize[0] * pixHeight / 4);
                            memset(d3, 0, frame->linesize[0] * pixHeight / 4);
                        }

                        videoFrame.timebase = (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;
                    }

                    videoFrame.width1 = pixWidth;
                    videoFrame.height1 = pixHeight;
                    videoFrame.linesize1 = frame->linesize[0];

                    videoFrame.width2 = pixWidth / 2;
                    videoFrame.height2 = pixHeight / 2;
                    videoFrame.linesize2 = frame->linesize[1];

                    videoFrame.width3 = pixWidth / 2;
                    videoFrame.height3 = pixHeight / 2;
                    videoFrame.linesize3 = frame->linesize[2];

                    // 根据实际数量，动态修改等待时间
                    size_t size = mQueueVideoFrame.size();
                    milliseconds = (int)size * 100 * frame->pkt_duration * (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;

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

                    if (frame->best_effort_timestamp < 0) break;

                    swrCtx = swr_alloc_set_opts(nullptr, frame->channel_layout, AV_SAMPLE_FMT_FLT, codeCtxAudio->sample_rate, codeCtxAudio->channel_layout, codeCtxAudio->sample_fmt, codeCtxAudio->sample_rate, 0, nullptr);
                    swr_init(swrCtx);
                    int bufsize = av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, AV_SAMPLE_FMT_FLT, 0);
                    uint8_t *buf = new uint8_t[bufsize];
                    const int out_num_samples = av_rescale_rnd(swr_get_delay(swrCtx, frame->sample_rate) + frame->nb_samples, frame->sample_rate, frame->sample_rate, AV_ROUND_UP);
                    int tmpSize = swr_convert(swrCtx, &buf, out_num_samples, (const uint8_t**)(frame->data), frame->nb_samples);
                    if (tmpSize <= 0) break;

                    AudioFrame audio = { frame->pts, bufsize, 0, buf };
                    audio.timebase = (double)codeCtxAudio->pkt_timebase.num / codeCtxAudio->pkt_timebase.den;
                    mQueueAudioFrame.push(audio);
                    mCvPlayMedia.notify_all();
                    milliseconds = 0;

                    av_frame_unref(frame);
                }

                av_packet_unref(packet);
                swr_free(&swrCtx);
            }
            else
            {
                milliseconds = 0;
            }
        }
    }

    av_frame_free(&frame);
    av_packet_unref(packet);
    av_packet_free(&packet);
    avcodec_free_context(&codeCtxVideo);
    avcodec_free_context(&codeCtxAudio);

    qDebug() << "parse media over ";
}

void WidgetPlayer::playVideoFrame()
{
    while (mMediaPlayFlag)
    {
        VideoFrame frame;
        if (mQueueVideoFrame.empty() || mMediaPauseFlag) continue;
        mQueueVideoFrame.wait_and_pop(frame);

        uint64_t currentTimeStamp = getCurrentMillisecond();
        uint64_t time = frame.pts * frame.timebase * 1000;

        if (mStartTimeStamp == 0) mStartTimeStamp = currentTimeStamp - time;

        if ((currentTimeStamp - mStartTimeStamp) < time) continue;

        mQueueVideoFrame.wait_and_pop();

        double rate = (double)frame.height / frame.width;
        double widthSize = this->width();
        double heightSize = this->width() * rate;

        if (this->height() < heightSize)
        {
            heightSize = this->height();
            widthSize = heightSize / rate;
        }

        mVideoWidth = widthSize;
        mVideoHeight = heightSize;

        std::unique_lock<std::mutex> lock(mMutexPlayFrame);
        if (mCurrentVideoFrame.pts > 0)
        {
            delete [] mCurrentVideoFrame.d1;
            delete [] mCurrentVideoFrame.d2;
            delete [] mCurrentVideoFrame.d3;
        }

        mCurrentVideoFrame = frame;
        lock.unlock();

        emit sgl_thead_update_video_frame();

        // 更新视频当前播放时长（frame . pts 不一定从 0 开始）
        emit AppSignal::getInstance()->sgl_current_video_frame_time(frame.pts * frame.timebase);
    }

    qDebug() << "play video over ";
}

void WidgetPlayer::playAudioFrame()
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(mSampleRate);
    audioFormat.setChannelCount(mAudioChannles);
    audioFormat.setSampleSize(8 * mSampleSize);
    audioFormat.setSampleType(QAudioFormat::Float);

    audioFormat.setCodec("audio/pcm");

    if (nullptr == mAudioOutput)
    {
        mAudioOutput = new QAudioOutput(audioFormat);
        mAudioVolume = SoftConfig::getInstance()->getValue("Volume", "value").toUInt() * 1.0 / 100;
        mAudioOutput->setVolume(mAudioVolume);
    }

    mAudioOutput->setBufferSize(mSampleRate * mAudioChannles * mSampleSize);
    mIODevice = mAudioOutput->start();

    // 提取音频帧
    while (mMediaPlayFlag)
    {
        AudioFrame frame;
        if (mQueueAudioFrame.empty() || mMediaPauseFlag) continue;
        mQueueAudioFrame.wait_and_pop(frame);

        uint64_t currentTimeStamp = getCurrentMillisecond();
        uint64_t time = frame.pts * frame.timebase * 1000;
        if (mStartTimeStamp == 0) mStartTimeStamp = currentTimeStamp - time;
        if ((currentTimeStamp - mStartTimeStamp) < time) continue;

        mQueueAudioFrame.wait_and_pop();

        if (mCurrentAudioFrame.pts > 0)
        {
            delete [] mCurrentAudioFrame.data;
        }

        mCurrentAudioFrame = frame;

        if (nullptr == frame.data) return;
        if (nullptr != mIODevice && mAudioVolume > 0)
        {
           mIODevice->write((const char*)frame.data, frame.size);
        }
    }

    qDebug() << "play audio over ";
}

uint64_t WidgetPlayer::getCurrentMillisecond()
{
    return (double)std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den * std::chrono::system_clock::period::num * 1000;
}
