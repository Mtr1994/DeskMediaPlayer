#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init();


protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void slot_start_play_video();

    void slot_pause_play_video();

    void slot_change_audio_volume(int volume);

    void slot_seek_video_position(int position);

    void slot_show_about_developer();

    void slot_open_video_file();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
