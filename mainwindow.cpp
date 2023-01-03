#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Public/appsignal.h"
#include "Configure/softconfig.h"
#include "Dialog/dialogversion.h"

#include <QScreen>
#include <Windows.h>
#include <QKeyEvent>
#include <QFileDialog>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    init();

    setWindowTitle("Mtr1994 Player");
}

MainWindow::~MainWindow()
{
    SoftConfig::getInstance()->setValue("Media", "path", "");
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

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::slot_open_video_file);
    connect(ui->actionQuit, &QAction::triggered, this, [this]{ this->close(); });

    connect(ui->actionSetting, &QAction::triggered, this, []{ /* 还没写，哈哈 */});

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::slot_show_about_developer);

    connect(AppSignal::getInstance(), &AppSignal::sgl_start_play_video, this, &MainWindow::slot_start_play_video);
    connect(AppSignal::getInstance(), &AppSignal::sgl_pause_play_video, this, &MainWindow::slot_pause_play_video);
    connect(AppSignal::getInstance(), &AppSignal::sgl_change_audio_volume, this, &MainWindow::slot_change_audio_volume);
    connect(AppSignal::getInstance(), &AppSignal::sgl_seek_video_position, this, &MainWindow::slot_seek_video_position);

    // 默认测试播放
    ui->widgetOpenGLPlayer->play(SoftConfig::getInstance()->getValue("Media", "path"));
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

void MainWindow::slot_seek_video_position(int position)
{
    ui->widgetOpenGLPlayer->seek(position);
}

void MainWindow::slot_show_about_developer()
{
    DialogVersion dialog(this);
    dialog.exec();
}

void MainWindow::slot_open_video_file()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, tr("选择视频文件"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), tr("视频文件 (*.mp4 *.mkv *.avi *.mov *.flv *.wmv *.mpg)"));
    if (fileName.isEmpty()) return;

    ui->widgetOpenGLPlayer->play(fileName);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers() == Qt::Modifier::CTRL)
    {
        if (event->key() == Qt::Key_O)
        {
            emit ui->actionOpen->triggered(true);
        }
    }
}
