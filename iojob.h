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
    QPair<Mode, QString> getMode();
    static QPair<Mode, QString> mode( const QStringList &files );

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
    void setInfo( QString from, QString to, int completeProgress, int currentProgress );
    inline void setSpeed( const QString &speed ) { m_speedLabel->setText(speed); }
    void finished();
    inline void pauseToggled( const bool pause, const bool cut ) { setWindowTitle(cut ? (pause ? "Moving... [paused]" : "Moving..." ) : (pause ? "Copying... [paused]" : "Copying... ")); }

private:
    QProgressBar *m_progress, *m_fileProgress;
    QPushButton *m_ok, *m_cancel, *m_pause;
    QLabel *m_from, *m_inFile, *m_to, *m_speedLabel;
    QCheckBox *m_cbHideFinished;
    bool m_paused, m_hideFinished;

private slots:
    void pauseCLicked();
    void finishedToggled(bool enabled);
};

//-----------------------------------------------------------------------------------------------

class IOThread : public QThread
{
    Q_OBJECT
public:
    enum Type { Copy = 0, Remove };
    IOThread(const QStringList &inf, const QString &dest, const bool cut, const quint64 totalSize, QObject *parent = 0);
    IOThread(const QStringList &paths = QStringList(), QObject *parent = 0);
    void run();
    bool paused() { return m_pause; }
    void setMode( QPair<Mode, QString> mode );

public slots:
    inline void cancelCopy() { m_canceled = true; m_inFiles.clear(); setPaused(false); qDebug() << "cancelling copy..."; }
    void setPaused(bool pause);

signals:
    void copyProgress(const QString &inFile, const QString &outFile, const int progress, const int currentProgress);
    void fileExists(const QStringList &files);
    void pauseToggled( const bool pause, const bool cut );
    void errorSignal();
    void speed( const QString &speed );

private slots:
    void fileExistsSlot( const QStringList &files ) { setMode(FileExistsDialog::mode(files)); }
    void emitProgress();
    void errorSlot();
    void finishedSlot() { emit copyProgress(QString(), QString(), 100, 100); emit speed(QString()); }
    void checkSpeed();

protected:
    void copyRecursive(const QString &inFile, const QString &outFile, bool cut, bool sameDisk);
    bool clone(const QString &in, const QString &out);
    bool remove(const QString &path) const;
    int currentProgress() { return m_allProgress*100/m_total; }

private:
    QStringList m_inFiles, m_rmPaths;
    QString m_destDir, m_inFile, m_newFile, m_outFile;
    bool m_cut, m_canceled, m_pause;
    quint64 m_total, m_allProgress, m_diffCheck, m_inSize;
    QMutex m_mutex;
    QWaitCondition m_pauseCond;
    Mode m_mode;
    Type m_type;
    int m_inProgress, m_fileProgress;
    QTimer *m_timer, *m_speedTimer;
};

//-----------------------------------------------------------------------------------------------

class Job : public QObject
{
    Q_OBJECT
public:
    inline explicit Job(QObject *parent = 0) : QObject(parent){}
    ~Job() { qDebug() << "destroyed"; }
    static void copy(const QStringList &sourceFiles, const QString &destination, bool cut = false, bool ask = false);
    static void copy(const QList<QUrl> &sourceFiles, const QString & destination, bool cut = false, bool ask = false);
    static void remove(const QStringList &paths);
    void getDirs(const QString &dir, quint64 *fileSize);
    void cp(const QStringList &copyFiles, const QString &destination, bool cut = false, bool ask = false);
    void cp(const QList<QUrl> &copyFiles, const QString &destination, bool cut = false, bool ask = false);
    void rm(const QStringList &paths) { (new IOThread(paths, this))->start(); }
};

}

}


#endif // IOJOB_H
