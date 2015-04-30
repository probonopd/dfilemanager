/**************************************************************************
*   Copyright (C) 2013 by Robert Metsaranta                               *
*   therealestrob@gmail.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#ifndef IOJOB_H
#define IOJOB_H

#include <QQueue>
#include <QObject>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>
#include <QLineEdit>
#include <QMutex>
#include <QKeyEvent>
#include "operations.h"
#include "globals.h"
#include "objects.h"

namespace DFM
{

namespace IO
{

enum Mode{ Cancel, Overwrite, OverwriteAll, NewName, Skip, SkipAll, Continue };

class FileExistsDialog : public QDialog
{
    Q_OBJECT
public:
    FileExistsDialog(const QStringList &files, QWidget *parent = 0);
    QString newName() const { return m_newFileName; }
    Mode getMode();

protected:
    bool event(QEvent *e);

protected slots:
    void accept();
    void textChanged(const QString &text);

private:
    QPushButton *m_overWrite, *m_overWriteAll, *m_skip, *m_skipAll, *m_newName;
    QLineEdit *m_edit;
    QLabel *m_name;
    QString m_file, m_newFileName;
    Mode m_mode;   
};

//-----------------------------------------------------------------------------------------------

class CopyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CopyDialog(QWidget *parent = 0);

signals:
    void pauseRequest(bool paused);

public slots:
    void setInfo(QString from, QString to, int completeProgress, int currentProgress);
    inline void setMove(const bool move) { m_cut = move; }
    inline void setSpeed(const QString &speed) { m_speedLabel->setText(speed); }
    void finished();
    void pauseToggled(const bool pause);

private:
    QProgressBar *m_progress, *m_fileProgress;
    QPushButton *m_ok, *m_cancel, *m_pause;
    QLabel *m_from, *m_inFile, *m_to, *m_speedLabel;
    QCheckBox *m_cbHideFinished;
    bool m_paused, m_hideFinished, m_cut;

private slots:
    void pauseCLicked();
    void finishedToggled(bool enabled);
};

//-----------------------------------------------------------------------------------------------

class Manager : public DThread
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
    ~Manager();

    void setMode(Mode mode);

    static void copy(const QStringList &sourceFiles, const QString &destination, bool cut = false, bool ask = false);
    static void copy(const QList<QUrl> &sourceFiles, const QString &destination, bool cut = false, bool ask = false);
    static void remove(const QStringList &files);
    inline QString errorString() const { return m_errorString; }

    bool hasQueue() const;
    int queueCount() const;
    void queue(const IOJobData &ioJob);

public slots:
    inline void cancelCopy() { m_canceled = true; setPause(false); qDebug() << "cancelling copy..."; }
    void getMessage(const QStringList &message);

signals:
    void copyOrMoveStarted();
    void copyOrMoveFinished();
    void copyProgress(const QString &inFile, const QString &outFile, const int progress, const int currentProgress);
    void fileExists(const QStringList &files);
    void errorSignal();
    void speed(const QString &speed);
    void isMove(const bool move);
    void ioIsBusy(const bool isBusy);

private slots:
    void fileExistsSlot(const QStringList &files);
    void emitProgress();
    void errorSlot();
    void finishedSlot();
    void checkSpeed();
    void ioBusy(const bool busy);

protected:
    bool copyRecursive(const QString &inFile, const QString &outFile, bool cut, bool sameDisk);
    bool clone(const QString &in, const QString &out);
    bool remove(const QString &path) const;
    int currentProgress() { return m_allProgress*100/m_total; }
    void reset();
    void doJob(const IOJobData &ioJobData);
    void getDirs(const QString &dir, quint64 &fileSize = (quint64 &)defaultInteger);
    bool getTotalSize(const QStringList &copyFiles, quint64 &fileSize = (quint64 &)defaultInteger);
    void error(const QString &error);
    IOJobData deqeueue();
    void run();

private:
    QString m_destDir, m_inFile, m_newFile, m_outFile, m_errorString;
    bool m_cut, m_canceled;
    quint64 m_total, m_allProgress, m_diffCheck;
    mutable QMutex m_queueMtx;
    Mode m_mode;
    int m_inProgress, m_fileProgress;
    QTimer *m_timer, *m_speedTimer;
    CopyDialog *m_copyDialog;
    QQueue<IOJobData> m_queue;
    IOJobData m_currentJob;
};

}

}


#endif // IOJOB_H
