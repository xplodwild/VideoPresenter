#include "screenform.h"
#include "ui_screenform.h"
#include "mainwindow.h"
#include <QByteArray>
#include <QPainter>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFile>
#include "frameplayer.h"

enum FrameFlags
{
    FLAG_ORIENTATION_0      = (0),
    FLAG_ORIENTATION_90     = (1 << 0),
    FLAG_ORIENTATION_180    = (1 << 1),
    FLAG_ORIENTATION_270    = (1 << 2),
    FLAG_SCREEN_OFF         = (1 << 3),
    FLAG_FRAME_HERO         = (1 << 4),
    FLAG_UNUSED_2           = (1 << 5),
    FLAG_UNUSED_3           = (1 << 6),
    FLAG_UNUSED_4           = (1 << 7)
};

//----------------------------------------------------
ScreenForm::ScreenForm(FramePlayer* fpParent, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScreenForm),
    mLastFrameId(0),
    mTotalFrameReceived(0),
    mInitialFpsAverage(0),
    mRotationAngle(0),
    mPreviousBufferCompleted(true),
    mJpegReadCount(0),
    mParent(fpParent)
{
    ui->setupUi(this);

    setWindowFlags( Qt::CustomizeWindowHint | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint );
    setAttribute(Qt::WA_TranslucentBackground, true);

    // Start UDP client
    for (int i = 0; i < SOCK_COUNT; i++)
    {
        mUdpSocket[i].bind(9876+i);
        mUdpSocket[i].setReadBufferSize(1024 * 512);

        connect(&mUdpSocket[i], SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));
    }

    mFrameTimer.start();

    // Run pinger every 1 secs
    startTimer(1000);

    mFaderTimer = startTimer(25);

    mJpegBuffer = QByteArray();
    this->setWindowOpacity(0.0f);
}
//----------------------------------------------------
ScreenForm::~ScreenForm()
{
    for (int i = 0; i < SOCK_COUNT; i++)
        mUdpSocket[i].close();

    delete ui;
}
//----------------------------------------------------
void ScreenForm::connectTo(const QString &host)
{
    mHost = host;
    for (int i = 0; i < SOCK_COUNT; i++)
    {
        mUdpSocket[i].writeDatagram(QString("").toAscii(), QHostAddress(mHost), 9876+i);
        mUdpSocket[i].flush();
    }

    this->setWindowTitle("BBQScreen - " + host);
}
//----------------------------------------------------
void ScreenForm::setQuality(bool high)
{
    mHighQuality = high;
}
//----------------------------------------------------
void ScreenForm::processPendingDatagrams()
{
    QByteArray datagram;

    int currentSocket = 0;
    int count = 1;
    int lastSlice = 0;
    while (mUdpSocket[0].hasPendingDatagrams() || mUdpSocket[1].hasPendingDatagrams())
    {
        QUdpSocket* theSock = &mUdpSocket[currentSocket];
        currentSocket++;

        if (currentSocket == SOCK_COUNT)
            currentSocket = 0;

        if (!theSock->hasPendingDatagrams())
            continue;

        // Read the first pending datagram
        datagram.resize(theSock->pendingDatagramSize());
        theSock->readDatagram(datagram.data(), datagram.size());

        // Read header
        quint8 headerSize = 6, protVersion, orientation;
        quint16 height, width;
        quint8 jpegThreads, jpegSlice;
        protVersion = bytesToUInt8(datagram, 0);
        orientation = bytesToUInt8(datagram, 1);
        height = bytesToUInt16(datagram, 2);
        jpegThreads = bytesToUInt8(datagram, 4);
        jpegSlice = bytesToUInt8(datagram, 5);

        if (protVersion != 2)
        {
            QMessageBox::critical(this, "Version mismatch (server: " + QString::number((int)protVersion) + ", client: 2)", "The client version doesn't match the server's one. Either you updated the app on your Android device and you need to update the client, or your client is more recent than the Android app (which means that you have to wait a couple of minutes for the Play Store to show the updated app on your device). If problem persists, contact us at support@bbqdroid.org");
            close();
            return;
        }

        if (mPixmaps.size() != jpegThreads)
            mPixmaps.resize(jpegThreads);

        mRotationAngle = orientation * (-90);

        // Calculate FPS
        if (jpegSlice == 0) {
            int timeSinceLastFrame = mFrameTimer.elapsed();
            mFrameTime[mLastFrameId++] = timeSinceLastFrame;
            mFrameTimer.restart();

            if (mInitialFpsAverage < FPS_AVERAGE_SAMPLES)
                mInitialFpsAverage++;

            if (mLastFrameId == FPS_AVERAGE_SAMPLES)
                mLastFrameId = 0;

            float average = 0;
            for (int i = 0; i < mInitialFpsAverage; i++)
                average += mFrameTime[i];

            average /= (float)FPS_AVERAGE_SAMPLES;
            average = 1.0f/(average / 1000.0f);

            //ui->lblFps->setText("FPS: " + QString::number(average,'f',1));
            ui->lblFps->setText("");
            //qDebug() << average;
        }

        // Set screen picture
        mPixmaps[jpegSlice] = QPixmap();
        if (mPixmaps[jpegSlice].loadFromData(datagram.right(datagram.size() - headerSize), "JPEG"))
        {
            count++;
            // Update display only on ordered slices to reduce tearing
            if (jpegSlice == lastSlice+1 || jpegThreads == 1)
            {
                if (mFinalPixmap.width() != mPixmaps[jpegSlice].width() ||
                        mFinalPixmap.height() != height)
                {
                    mFinalPixmap = QPixmap(mPixmaps[jpegSlice].width(), height);
                }

                QPainter painter(&mFinalPixmap);

                // Draw the image
                for (int i = 0; i < mPixmaps.size(); i++)
                    painter.drawPixmap(0, mPixmaps[i].height() * i,
                                   mPixmaps[i].width(), mPixmaps[i].height(), mPixmaps[i]);

                painter.end();

                Qt::TransformationMode transformQuality = (mHighQuality ? Qt::SmoothTransformation : Qt::FastTransformation);

                if (mRotationAngle != 0)
                {
                    QTransform t;
                    t.rotate(mRotationAngle);
                    mFinalPixmap = mFinalPixmap.transformed(t, transformQuality);
                }

                ui->lblScreen->setPixmap(mFinalPixmap.scaled(ui->lblScreen->width(), ui->lblScreen->height(), Qt::KeepAspectRatio, transformQuality));
                //ui->lblScreen->setPixmap(mFinalPixmap);
                mTotalFrameReceived++;

                // Update the actual window
                qApp->processEvents();
            }

            lastSlice = jpegSlice;
        }
    }
}
//----------------------------------------------------
void ScreenForm::timerEvent(QTimerEvent *evt)
{
    if (evt->timerId() == mFaderTimer)
    {
        if (mTotalFrameReceived > 0)
        {
            this->setWindowOpacity(this->windowOpacity() + 0.1);
            if (this->windowOpacity() >= 1)
                killTimer(mFaderTimer);
        }
    }
    else
    {
        if (!isVisible())
        {
            for (int i = 0; i < SOCK_COUNT; i++)
                mUdpSocket[i].close();

            return;
        }

        // Send keep-alive
        for (int i = 0; i < SOCK_COUNT; i++)
        {
            mUdpSocket[i].writeDatagram(QString("1").toAscii(), QHostAddress(mHost), 9876+i);
            mUdpSocket[i].flush();
        }

        // Show message if no frames for a long time, or no frame at all
        if (mTotalFrameReceived == 0 && mFrameTimer.elapsed() > 3000)
        {
            ui->lblFps->setVisible(true);
            ui->lblFps->setText("<b>No frame received, are you sure you entered the correct IP address of your device?</b>");
        }
        else if (mFrameTimer.elapsed() > 5000)
        {
            ui->lblFps->setVisible(true);
            ui->lblFps->setText("<b>No frame received in last 5 seconds. Connection lost?</b>");
        }
        else
        {
            ui->lblFps->setVisible(false);
        }
    }
}
//----------------------------------------------------
quint8 ScreenForm::bytesToUInt8(const QByteArray& bytes, int offset)
{
    return ((unsigned char) bytes.at(offset));
}
//----------------------------------------------------
quint16 ScreenForm::bytesToUInt16(const QByteArray& bytes, int offset)
{
    quint16 value = 0;
    for (int i = 0; i < 2; i++) {
        value += ((unsigned char) bytes.at(offset + i)) << (8 * (1-i));
    }
    return value;
}
//----------------------------------------------------
void ScreenForm::closeEvent(QCloseEvent *evt)
{
    for (int i = 0; i < SOCK_COUNT; i++)
    {
        mUdpSocket[i].close();
    }

    mParent->setFocus();

    QWidget::closeEvent(evt);
}
//----------------------------------------------------
void ScreenForm::keyPressEvent(QKeyEvent *evt)
{
    if (evt->key() == Qt::Key_Escape || evt->key() == Qt::Key_F)
    {
        close();
        evt->accept();
    }
}
//----------------------------------------------------
