#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void timerEvent(QTimerEvent *evt);

protected:
    void _doSave(QString fileName);
    void _openClip(QString file);
    void _addKeyToTable(QTime start, QTime end, QString type);

private slots:
    void onClick_OpenClip();
    void onTick_VideoPreview(qint64 time);
    void onClick_AddKeyFrame();
    void onClick_RemoveKeyFrame();
    void onClick_Play();
    void onClick_PlayOnThisMonitor();
    void onClick_PlayOnOtherMonitor();
    void onClick_Save();
    void onClick_SaveAs();
    void onClick_OpenPresentation();
    void onClick_SetTime();
    void onClick_Goto();
    void onClick_StepLeft();
    void onClick_StepRight();

private:
    Ui::MainWindow *ui;
    QString m_VideoFile;
    QString m_PresentationFile;
    QTime m_TimeShow;
};

#endif // MAINWINDOW_H
