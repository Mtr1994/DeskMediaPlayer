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

    void slot_media_show_full_screen();

    void slot_thread_save_capture_status(bool status, const QString &path);

    void slot_system_output_message(const QString &message);

    void slot_start_play_target_media(const QString &path);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
