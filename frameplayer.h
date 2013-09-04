#ifndef FRAMEPLAYER_H
#define FRAMEPLAYER_H

#include <QWidget>

namespace Ui {
    class FramePlayer;
}

class FramePlayer : public QWidget
{
    Q_OBJECT

public:
    explicit FramePlayer(QWidget *parent = 0);
    ~FramePlayer();

    void keyPressEvent(QKeyEvent *evt);

    void setVideo(QString fileName);
    void start();
    void setOtherScreen();

private slots:
    void onTick_Video(qint64 time);

private:
    Ui::FramePlayer *ui;
    int m_CurrentKeyFrame;
    bool m_AllKeysDone;
    bool m_IsOtherScreen;
};

#endif // FRAMEPLAYER_H
