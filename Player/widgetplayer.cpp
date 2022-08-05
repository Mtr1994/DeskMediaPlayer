#include "widgetplayer.h"

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

// test
#include <QDebug>

WidgetPlayer::WidgetPlayer(QWidget *parent) : QOpenGLWidget(parent)
{
    mCurrentFrame.pts = -1;
}

WidgetPlayer::~WidgetPlayer()
{
    //glDeleteBuffers();
    //glDeleteProgram();
}

void WidgetPlayer::play(const QString &path)
{
    if (path.isEmpty()) return;

    // 线程更新视频界面
    connect(this, &WidgetPlayer::sgl_thead_update_video_frame, this, [this] { update();}, Qt::QueuedConnection);

    auto funcParse = std::bind(&WidgetPlayer::parse, this, std::placeholders::_1);
    std::thread threadParse(funcParse, path);
    threadParse.detach();

    // 开始播放
    start();
}

void WidgetPlayer::start()
{
    auto funcStart= std::bind(&WidgetPlayer::playVideoFrame, this);
    std::thread threadStart(funcStart);
    threadStart.detach();
}

void WidgetPlayer::pause()
{

}

void WidgetPlayer::stop()
{

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

    if (mCurrentFrame.pts < 0) return;

    mShaderProgram.bind();
    mVertexBufferObject.bind();
    mShaderProgram.enableAttributeArray("vertexIn");
    mShaderProgram.enableAttributeArray("textureIn");
    mShaderProgram.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    mShaderProgram.setAttributeBuffer("textureIn", GL_FLOAT, 2 * 4 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    std::unique_lock<std::mutex> lock(mMutexPlayFrame);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureArray[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentFrame.linesize1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mCurrentFrame.width1, mCurrentFrame.height1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mCurrentFrame.d1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureArray[1]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentFrame.linesize2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mCurrentFrame.width2, mCurrentFrame.height2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mCurrentFrame.d2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTextureArray[2]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, mCurrentFrame.linesize3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mCurrentFrame.width3, mCurrentFrame.height3, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mCurrentFrame.d3);
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

    if (mCurrentFrame.pts < 0)
    {
        mVideoWidth = 0;
        mVideoHeight = 0;
        return;
    }

    double rate = (double)mCurrentFrame.height / mCurrentFrame.width;
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
        }
    }

    auto sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
    // 分配一个packet
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int milliseconds = 0;

    std::mutex mutexParse;
    std::condition_variable cvParse;
    while (true)
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

                    int pixWidth = frame->width;
                    int pixHeight = frame->height;

                    VideoFrame videoFrame = { frame->pts, pixWidth, pixHeight, sampleSize, 0, nullptr, 0, 0, 0, nullptr, 0, 0, 0, nullptr, 0, 0, 0 };
                    if (frame->format == AV_PIX_FMT_YUV420P)
                    {
                        uint8_t *d1 = new uint8_t[frame->linesize[0] * pixHeight];
                        uint8_t *d2 = new uint8_t[frame->linesize[0] * pixHeight / 4];
                        uint8_t *d3 = new uint8_t[frame->linesize[0] * pixHeight / 4];

                        memcpy(d1, frame->data[0], frame->linesize[0] * pixHeight);
                        memcpy(d2, frame->data[1], frame->linesize[0] * pixHeight / 4);
                        memcpy(d3, frame->data[2], frame->linesize[0] * pixHeight / 4);

                        videoFrame.timebase = (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;

                        videoFrame.d1 = d1;
                        videoFrame.width1 = pixWidth;
                        videoFrame.height1 = pixHeight;
                        videoFrame.linesize1 = frame->linesize[0];

                        videoFrame.d2 = d2;
                        videoFrame.width2 = pixWidth / 2;
                        videoFrame.height2 = pixHeight / 2;
                        videoFrame.linesize2 = frame->linesize[1];

                        videoFrame.d3 = d3;
                        videoFrame.width3 = pixWidth / 2;
                        videoFrame.height3 = pixHeight / 2;
                        videoFrame.linesize3 = frame->linesize[2];
                    }

                    // 根据实际数量，动态修改等待时间
                    size_t size = mQueueVideoFrame.size();
                    milliseconds = (int)size * 100 * frame->pkt_duration * (double)codeCtxVideo->pkt_timebase.num / codeCtxVideo->pkt_timebase.den;

                    mQueueVideoFrame.push(videoFrame);

                    // 通知播放
                    mCvPlayMedia.notify_all();

                    av_frame_unref(frame);
                }
                av_packet_unref(packet);
            }
            else if (packet->stream_index == audioStreamIndex)
            {
                ret = avcodec_send_packet(codeCtxAudio, packet);
                if (ret < 0)
                {
                    av_packet_unref(packet);
                    continue;
                }
                AVFrame *frame = av_frame_alloc();
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

                    auto swrCtx = swr_alloc_set_opts(nullptr, frame->channel_layout, AV_SAMPLE_FMT_FLT, codeCtxAudio->sample_rate, codeCtxAudio->channel_layout, codeCtxAudio->sample_fmt, codeCtxAudio->sample_rate, 0, nullptr);
                    swr_init(swrCtx);
                    int bufsize = av_samples_get_buffer_size(frame->linesize, frame->channels, frame->nb_samples, AV_SAMPLE_FMT_FLT, 0);
                    uint8_t *buf = new uint8_t[bufsize];
                    const int out_num_samples = av_rescale_rnd(swr_get_delay(swrCtx, frame->sample_rate) + frame->nb_samples, frame->sample_rate, frame->sample_rate, AV_ROUND_UP);
                    int tmpSize = swr_convert(swrCtx, &buf, out_num_samples, (const uint8_t**)(frame->data), frame->nb_samples);
                    if (tmpSize > 0)
                    {
                       AudioFrame audio =
                        {
                            frame->pts,
                            bufsize,
                            (double)codeCtxAudio->pkt_timebase.num / codeCtxAudio->pkt_timebase.den,
                            buf
                        };

                       mQueueAudioFrame.push(audio);

                       mCvPlayMedia.notify_all();
                    }
                    else
                    {
                        qDebug() << "audio  convert failed";
                    }

                    swr_free(&swrCtx);

                    milliseconds = 0;
                }
                av_packet_unref(packet);
            }
            else
            {
                qDebug() << "other stream";
                milliseconds = 0;
            }
        }
    }

    av_frame_free(&frame);
    av_packet_unref(packet);
    av_packet_free(&packet);
    avcodec_free_context(&codeCtxVideo);
    avcodec_free_context(&codeCtxAudio);
}

void WidgetPlayer::playVideoFrame()
{
    uint64_t startTime = getCurrentMillisecond();
    while (true)
    {
        VideoFrame frame;
        if (mQueueVideoFrame.empty()) continue;
        mQueueVideoFrame.wait_and_pop(frame);

        int64_t currentTime = getCurrentMillisecond();
        int time = frame.pts * frame.timebase * 1000;

        if (int(currentTime - startTime) < time) continue;

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
        if (mCurrentFrame.pts > 0)
        {
            delete [] mCurrentFrame.d1;
            delete [] mCurrentFrame.d2;
            delete [] mCurrentFrame.d3;
        }

        mCurrentFrame = frame;
        lock.unlock();

        emit sgl_thead_update_video_frame();
    }

    qDebug() << "play video over ";
}

uint64_t WidgetPlayer::getCurrentMillisecond()
{
    return (double)std::chrono::system_clock::now().time_since_epoch().count() / std::chrono::system_clock::period::den * std::chrono::system_clock::period::num * 1000;
}
