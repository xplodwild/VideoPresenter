#include "frameplayer.h"
#include "ui_frameplayer.h"
#include <QKeyEvent>
#include <QDesktopWidget>
#include <Phonon>
#include "KeyFrameHolder.h"
#include "screenform.h"

//--------------------------------------------------------
FramePlayer::FramePlayer(QWidget *parent) :
    QWidget(parent, Qt::CustomizeWindowHint | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
    ui(new Ui::FramePlayer), m_IsOtherScreen(false)
{
    ui->setupUi(this);
    this->setFocus();

    connect(ui->videoPlayer->mediaObject(), SIGNAL(tick(qint64)), this, SLOT(onTick_Video(qint64)));
    ui->videoPlayer->mediaObject()->setTickInterval(15);

    // videoplayer never gets the focus so the QWidget gets all keypresses
    ui->videoPlayer->setFocusPolicy(Qt::NoFocus);

    // Fullsize me on default screen first !
    QRect screenres = QApplication::desktop()->screenGeometry(0);
    this->move(QPoint(screenres.x(), screenres.y()));
    this->resize(screenres.size());

    // hide mouse
    setCursor( QCursor( Qt::BlankCursor ) );
    ui->videoPlayer->setCursor( QCursor( Qt::BlankCursor ) );

}
//--------------------------------------------------------
FramePlayer::~FramePlayer()
{
    delete ui;
}
//--------------------------------------------------------
void FramePlayer::keyPressEvent(QKeyEvent *evt)
{
    if (evt->key() == Qt::Key_Escape)
    {
        ui->videoPlayer->stop();
        ui->videoPlayer->deleteLater();
        delete ui;
        this->close();
    }
    else if (evt->key() == Qt::Key_Right || evt->key() == Qt::Key_Return)
    {
        ui->videoPlayer->play();
    }
    else if (evt->key() == Qt::Key_Left)
    {
        if (m_CurrentKeyFrame > 0)
            m_CurrentKeyFrame--;
        qDebug() << "Seeking to frame " << m_CurrentKeyFrame;

        ui->videoPlayer->seek(QTime().msecsTo(KeyFrameHolder::m_Pauses[m_CurrentKeyFrame]) - 1);
    }
    else if (evt->key() == Qt::Key_N)
    {
        if (KeyFrameHolder::m_Pauses.size() == m_CurrentKeyFrame+1)
            m_AllKeysDone = true;
        else
        {
            m_CurrentKeyFrame++;
            ui->videoPlayer->seek(QTime().msecsTo(KeyFrameHolder::m_Pauses[m_CurrentKeyFrame]) - 1);
            qDebug() << "Seeking to frame " << m_CurrentKeyFrame;
        }
    }
    else if (evt->key() == Qt::Key_Space)
    {
        if (ui->videoPlayer->isPaused())
            ui->videoPlayer->play();
        else
            ui->videoPlayer->pause();
    }
    else if (evt->key() == Qt::Key_F || evt->key() == Qt::Key_B)
    {
        ScreenForm* sf = new ScreenForm(this);
        sf->show();
        sf->setQuality(true);
        sf->connectTo("192.168.42.129");

        if (m_IsOtherScreen)
        {
            QRect screenres = QApplication::desktop()->screenGeometry(1);
            sf->move(QPoint(screenres.x(), screenres.y() + 200));
            sf->resize(screenres.size().width(), screenres.size().height()-200);
        } else {
            QRect screenres = QApplication::desktop()->screenGeometry(0);
            sf->move(QPoint(screenres.x(), screenres.y() + 200));
            sf->resize(screenres.size().width(), screenres.size().height()-200);
        }

        sf->setFocus();
    }
}
//--------------------------------------------------------
void FramePlayer::setVideo(QString fileName)
{
    ui->videoPlayer->load(Phonon::MediaSource(fileName));
    ui->videoPlayer->play();
    ui->videoPlayer->pause();
}
//--------------------------------------------------------
void FramePlayer::start()
{
    ui->videoPlayer->play();
    m_CurrentKeyFrame = 0;
    m_AllKeysDone = false;
}
//--------------------------------------------------------
void FramePlayer::onTick_Video(qint64 time)
{
    if (!m_AllKeysDone && KeyFrameHolder::m_Pauses[m_CurrentKeyFrame] <= QTime().addMSecs(time))
    {
        ui->videoPlayer->pause();
        ui->videoPlayer->mediaObject()->seek(QTime(0,0,0).msecsTo(KeyFrameHolder::m_Pauses[m_CurrentKeyFrame]));

        if (KeyFrameHolder::m_Pauses.size() == m_CurrentKeyFrame+1)
            m_AllKeysDone = true;
        else
            m_CurrentKeyFrame++;

        qDebug() << "Frame set: " << m_CurrentKeyFrame;
    }
}
//--------------------------------------------------------
void FramePlayer::setOtherScreen()
{
    QRect screenres = QApplication::desktop()->screenGeometry(1);
    this->move(QPoint(screenres.x(), screenres.y()));
    this->resize(screenres.size());
    m_IsOtherScreen = true;
}
