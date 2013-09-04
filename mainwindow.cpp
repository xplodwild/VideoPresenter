#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <phonon/videoplayer.h>
#include <phonon/mediaobject.h>
#include <QFileDialog>
#include <QTime>
#include "KeyFrameHolder.h"
#include "frameplayer.h"
#include <QMessageBox>
#include <QProgressDialog>
#include <QProgressBar>

//-------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionOuvrir_un_clip, SIGNAL(triggered()), this, SLOT(onClick_OpenClip()));
    connect(ui->actionSur_ce_moniteur, SIGNAL(triggered()), this, SLOT(onClick_PlayOnThisMonitor()));
    connect(ui->actionSur_le_second_moniteur, SIGNAL(triggered()), this, SLOT(onClick_PlayOnOtherMonitor()));
    connect(ui->actionEnregistrer_la_pr_sentation, SIGNAL(triggered()), this, SLOT(onClick_Save()));
    connect(ui->actionEnregistrer_la_pr_sentation_sous, SIGNAL(triggered()), this, SLOT(onClick_SaveAs()));
    connect(ui->actionOuvrir_une_pr_sentation, SIGNAL(triggered()), this, SLOT(onClick_OpenPresentation()));

    connect(ui->btn_AddKeyFrame, SIGNAL(clicked()), this, SLOT(onClick_AddKeyFrame()));
    connect(ui->btn_RemoveKeyFrame, SIGNAL(clicked()), this, SLOT(onClick_RemoveKeyFrame()));
    connect(ui->btn_PlayPause, SIGNAL(clicked()), this, SLOT(onClick_Play()));
    connect(ui->btn_SetTimeIn, SIGNAL(clicked()), this, SLOT(onClick_SetTime()));
    connect(ui->btn_Goto, SIGNAL(clicked()), this, SLOT(onClick_Goto()));
    connect(ui->btn_StepLeft, SIGNAL(clicked()), this, SLOT(onClick_StepLeft()));
    connect(ui->btn_StepRight, SIGNAL(clicked()), this, SLOT(onClick_StepRight()));

    connect(ui->video_Preview->mediaObject(), SIGNAL(tick(qint64)), this, SLOT(onTick_VideoPreview(qint64)));

    ui->lbl_TimeShow->setVisible(false);

    this->setWindowState(Qt::WindowMaximized);
}
//-------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}
//-------------------------------------------------------------
void MainWindow::onClick_OpenClip()
{
    QString url = QFileDialog::getOpenFileName(this, "Vidéo", "", "*.mov; *.avi; *.wmv; *.mpg; *.mp4");

    if (url != "")
        _openClip(url);
}
//-------------------------------------------------------------
void MainWindow::_openClip(QString file)
{
    m_VideoFile = file;

    ui->video_Preview->load(Phonon::MediaSource(file));
    ui->video_Preview->play();
    ui->video_Preview->pause();
    ui->video_Preview->mediaObject()->setTickInterval(40);

    ui->seek_Preview->setMediaObject(ui->video_Preview->mediaObject());
    ui->seek_Preview->setTracking(true);
}
//-------------------------------------------------------------
void MainWindow::onTick_VideoPreview(qint64 time)
{
    ui->lbl_TimePosition->setText(QTime().addMSecs(time).toString("HH:mm:ss.zzz") + " / " + QTime().addMSecs(ui->video_Preview->mediaObject()->totalTime()).toString("HH:mm:ss.zzz"));
}
//-------------------------------------------------------------
void MainWindow::onClick_AddKeyFrame()
{
    QTime pauseTime = QTime().addMSecs(ui->video_Preview->mediaObject()->currentTime());
    // Ask key type - ToDo
    KeyFrameHolder::m_Pauses.push_back(pauseTime);
    _addKeyToTable(pauseTime,pauseTime,"Pause");
}
//-------------------------------------------------------------
void MainWindow::_addKeyToTable(QTime start, QTime end, QString type)
{
    ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 0, new QTableWidgetItem(start.toString("HH:mm:ss.zzz")));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 1, new QTableWidgetItem(end.toString("HH:mm:ss.zzz")));
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 2, new QTableWidgetItem(type));

    KeyFrameHolder::reorder();
    ui->tableWidget->sortByColumn(0,Qt::AscendingOrder);
    ui->tableWidget->resizeColumnsToContents();
}
//-------------------------------------------------------------
void MainWindow::onClick_RemoveKeyFrame()
{

}
//-------------------------------------------------------------
void MainWindow::onClick_Play()
{
    if (ui->video_Preview->isPaused())
    {
        ui->video_Preview->play();
        ui->btn_PlayPause->setText("Pause");
    }
    else
    {
        ui->video_Preview->pause();
        ui->btn_PlayPause->setText("Play");
    }

}
//-------------------------------------------------------------
void MainWindow::onClick_PlayOnThisMonitor()
{
    if (m_VideoFile == "")
    {
        QMessageBox::critical(this, "Aucun fichier vidéo!", "Aucun fichier vidéo n'est chargé !");
        return;
    }

    startTimer(10);
    m_TimeShow.start();

    FramePlayer* fp = new FramePlayer;
    fp->setVideo(m_VideoFile);
    fp->show();
    fp->start();
}
//-------------------------------------------------------------
void MainWindow::onClick_PlayOnOtherMonitor()
{
    if (m_VideoFile == "")
    {
        QMessageBox::critical(this, "Aucun fichier vidéo!", "Aucun fichier vidéo n'est chargé !");
        return;
    }

    startTimer(10);
    m_TimeShow.start();

    FramePlayer* fp = new FramePlayer;
    fp->setVideo(m_VideoFile);
    fp->setOtherScreen();
    fp->show();
    fp->start();
}
//-------------------------------------------------------------
void MainWindow::onClick_Save()
{
    if (m_PresentationFile == "")
        onClick_SaveAs();

    _doSave(m_PresentationFile);
}
//-------------------------------------------------------------
void MainWindow::onClick_SaveAs()
{
    QString fileLocation = QFileDialog::getSaveFileName(this, "Enregistrer sous...", ".", "*.vp");

    if (fileLocation != "")
        _doSave(fileLocation);
}
//-------------------------------------------------------------
void MainWindow::_doSave(QString fileName)
{
    m_PresentationFile = fileName;

    QFile* saveFile = new QFile(fileName);
    saveFile->open(QFile::WriteOnly);

    QString data = m_VideoFile + "\n";

    for (int i = 0; i < KeyFrameHolder::m_Pauses.size(); i++)
    {
        data += KeyFrameHolder::m_Pauses[i].toString("HH:mm:ss.zzz") + "\n";
    }

    saveFile->write(data.toAscii());
    saveFile->close();

    QMessageBox::information(this, "Sauvegarde terminée", "Sauvegardé !");
}
//-------------------------------------------------------------
void MainWindow::onClick_OpenPresentation()
{
    QString file = QFileDialog::getOpenFileName(this, "Ouvrir une présentation...", ".", "*.vp");

    if (file != "")
    {
        m_PresentationFile = file;
        KeyFrameHolder::m_Pauses.clear();

        // start reading file
        QFile openFile(file);

        if (!openFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::critical(this, "Erreur", "Impossible d'ouvrir le fichier " + file + " !\n" + openFile.errorString());
            return;
        }

        QProgressDialog* qpd = new QProgressDialog();
        qpd->show();
        qpd->setCancelButton(0);
        qpd->setLabelText("Ouverture du fichier en cours...");
        qApp->processEvents();

        QTextStream in(&openFile);
        QString line = in.readLine();

        int lineNumber = 0;
        while (!line.isNull())
        {
            if (lineNumber == 0)
            {
                 _openClip(line);
                 qpd->setValue(50);
                 qApp->processEvents();
            }
            else
            {
                QTime time = QTime::fromString(line, "HH:mm:ss.zzz");
                KeyFrameHolder::m_Pauses.push_back(time);
                _addKeyToTable(time,time,"Pause");
            }

            lineNumber++;
            line = in.readLine();
        }

        qpd->close();
        delete qpd;
        qApp->processEvents();
    }

    setFocus();
}
//-------------------------------------------------------------
void MainWindow::onClick_SetTime()
{
    if (ui->tableWidget->selectedItems().size() == 0)
        return;

    QTableWidgetItem* item = (*(ui->tableWidget->selectedItems().begin()));

    QTime newTime = QTime().addMSecs(ui->video_Preview->mediaObject()->currentTime());

    KeyFrameHolder::m_Pauses[item->row()] = newTime;
    item->setText(newTime.toString("HH:mm:ss.zzz"));

    KeyFrameHolder::reorder();
    ui->tableWidget->sortByColumn(0);
}
//-------------------------------------------------------------
void MainWindow::onClick_Goto()
{
    if (ui->tableWidget->selectedItems().size() == 0)
        return;

    QTableWidgetItem* item = (*(ui->tableWidget->selectedItems().begin()));

    QTime theTime = QTime::fromString(item->text(), "HH:mm:ss.zzz");
    ui->video_Preview->seek((theTime.hour() * 3600 + theTime.minute() * 60 + theTime.second()) * 1000 + theTime.msec());
}
//-------------------------------------------------------------
void MainWindow::onClick_StepLeft()
{
    ui->video_Preview->seek(ui->video_Preview->mediaObject()->currentTime() - 30);
}
//-------------------------------------------------------------
void MainWindow::onClick_StepRight()
{
    ui->video_Preview->seek(ui->video_Preview->mediaObject()->currentTime() + 30);
}
//-------------------------------------------------------------
void MainWindow::timerEvent(QTimerEvent *evt)
{
    ui->lbl_TimeShow->setVisible(true);
    ui->lbl_TimeShow->setText(QTime(0,0).addMSecs(m_TimeShow.elapsed()).toString("HH:mm:ss"));
}
//-------------------------------------------------------------
