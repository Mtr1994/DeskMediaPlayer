#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Public/appsignal.h"

#include <QScreen>
#include <Windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("木头人视频播放器");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    QScreen *screen = QGuiApplication::screens().at(0);
    float width = 1024;
    float height = 640;

    if (nullptr != screen)
    {
        QRect rect = screen->availableGeometry();
        width = rect.width() * 0.64 < 1024 ? 1024 : rect.width() * 0.64;
        height = rect.height() * 0.64 < 640 ? 640 : rect.height() * 0.64;
    }

    resize(width, height);

    connect(AppSignal::getInstance(), &AppSignal::sgl_start_play_video, this, &MainWindow::slot_start_play_video);
    connect(AppSignal::getInstance(), &AppSignal::sgl_pause_play_video, this, &MainWindow::slot_pause_play_video);
    connect(AppSignal::getInstance(), &AppSignal::sgl_change_audio_volume, this, &MainWindow::slot_change_audio_volume);

    // 默认测试播放
    ui->widgetOpenGLPlayer->play("C:/Users/87482/Desktop/videos/demo2.mp4");
}

void MainWindow::slot_start_play_video()
{
    ui->widgetOpenGLPlayer->start();
}

void MainWindow::slot_pause_play_video()
{
    ui->widgetOpenGLPlayer->pause();
}

void MainWindow::slot_change_audio_volume(int volume)
{
    ui->widgetOpenGLPlayer->setAudioVolume(volume * 1.0 / 100.0);
}

