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

namespace DFM
{

namespace IO
{

enum Mode{ Cancel, Overwrite, OverwriteAll, NewName, Skip, SkipAll };

class FileExistsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileExistsDialog(QWidget *parent = 0);
    QString newName() { return m_file; }
    static void getNewInfo(Mode *m, QString *file);
public slots:
    Mode getMode(const QString &file);
protected:
    bool event(QEvent *);
signals:
    void fileExistsData( const Mode &m, const QString &newName );

private slots:
    void textChanged(const QString &text);

private:
    QPushButton *m_overWrite, *m_overWriteAll, *m_skip, *m_skipAll, *m_newName;
    QLineEdit *m_edit;
    QLabel *m_name;
    QString m_file;
    Mode m_mode;

private slots:
    void continueAction();
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
    void finished();
    void pauseToggled( const bool &pause, const bool &cut ) { setWindowTitle(cut ? (pause ? "Moving... [paused]" : "Moving..." ) : (pause ? "Copying... [paused]" : "Copying... ")); }

protected:
    void hideEvent(QHideEvent *);
    void showEvent(QShowEvent *);

private:
    QProgressBar *m_progress, *m_fileProgress;
    QPushButton *m_ok, *m_cancel, *m_pause;
    QLabel *m_from, *m_inFile, *m_to;
    QCheckBox *m_cbHideFinished;
    bool m_paused, m_hideFinished;

private slots:
    void pauseCLicked();
    void finishedToggled(bool enabled) {  m_hideFinished = enabled; }
};

//-----------------------------------------------------------------------------------------------

class IOThread : public QThread
{
    Q_OBJECT
public:
    enum Type { Copy = 0, Remove };
    explicit IOThread(const QStringList &inf, const QString &dest, const bool &cut, const qint64 &totalSize, QObject *parent = 0);
    explicit IOThread(const QStringList &paths = QStringList(), QObject *parent = 0) : QThread(parent), m_type(Remove), m_rmPaths(paths) { connect(this, SIGNAL(finished()), this, SLOT(deleteLater())); }
    void run();
    bool paused() { return m_pause; }

public slots:
    void cancelCopy() { m_canceled = true; qDebug() << "cancelling copy..."; }
    void setPaused(bool pause);
    void fileExistsAction(const Mode &m, const QString &file);

signals:
    void copyProgress(const QString &inFile, const QString &outFile, const int &progress, const int &currentProgress);
    void fileExists(const QString &file);
    void pauseToggled( const bool &pause, const bool &cut );

private:
    bool copyRecursive(const QString &inFile, const QString &outFile, bool cut, bool sameDisk);
    bool clone(const QString &in, const QString &out);
    bool remove(const QString &path) const;
    int currentProgress() { return int((((float)m_allProgress + m_fileProgress)/m_total)*100); }
    QStringList m_inFiles, m_rmPaths;
    QString m_destDir, m_inFile, m_newFile;
    bool m_cut, m_canceled;
    qint64 m_total, m_fileProgress, m_allProgress;
    QMutex m_mutex;
    QWaitCondition m_pauseCond;
    bool m_pause, m_overWriteAll, m_skipAll;
    Mode m_mode;
    Type m_type;
};

//-----------------------------------------------------------------------------------------------

class Job : public QObject
{
    Q_OBJECT
public:
    explicit Job(QObject *parent = 0);
    static inline void copy(const QStringList &sourceFiles, const QString &destination, const bool &cut = false) { instance()->cp(sourceFiles, destination, cut); }
    static inline void remove(const QStringList &paths) { instance()->rm(paths); }
    void getDirs(const QString &dir, quint64 *fileSize);
public slots:
protected:
    static Job *instance();
    void cp(const QStringList &copyFiles, const QString &destination, const bool &cut = false);
    void rm(const QStringList &paths) { (new IOThread(paths, this))->start(); }
private:
    quint64 m_fileSize, m_fileProgress;
    CopyDialog *m_copyDialog;
    FileExistsDialog *m_fileExistsDialog;
    bool m_canceled, m_cut;
    IOThread *m_thread;
};

}

}


#endif // IOJOB_H
