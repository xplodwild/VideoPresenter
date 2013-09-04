#ifndef SCREENFORM_H
#define SCREENFORM_H

#include <QWidget>
#include <QUdpSocket>
#include <QTime>
#include <QPainter>

#define FPS_AVERAGE_SAMPLES 50
#define SOCK_COUNT 2

namespace Ui {
class ScreenForm;
}

class FramePlayer;

class ScreenForm : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenForm(FramePlayer* fpParent, QWidget *parent = 0);
    ~ScreenForm();

    void connectTo(const QString& host);

    void timerEvent(QTimerEvent *evt);
    void closeEvent(QCloseEvent *evt);

    void keyPressEvent(QKeyEvent *evt);
    quint8 bytesToUInt8(const QByteArray& bytes, int offset);
    quint16 bytesToUInt16(const QByteArray& bytes, int offset);

    void setQuality(bool high);

private slots:
    void processPendingDatagrams();

private:
    Ui::ScreenForm *ui;
    FramePlayer* mParent;

    QUdpSocket mUdpSocket[SOCK_COUNT];
    int mFrameTime[FPS_AVERAGE_SAMPLES];
    int mInitialFpsAverage;
    int mLastFrameId;
    int mTotalFrameReceived;
    int mRotationAngle;
    QTime mFrameTimer;
    QString mHost;
    bool mHighQuality;

    QByteArray mJpegBuffer;
    bool mPreviousBufferCompleted;
    int mJpegReadCount;
    QPixmap mFinalPixmap;

    int mFaderTimer;

    QVector<QPixmap> mPixmaps;
};

#endif // SCREENFORM_H
