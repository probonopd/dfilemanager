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


#include <QDirIterator>

#include "iojob.h"
#include "operations.h"
#include "devicemanager.h"

#include "application.h"

using namespace DFM;
using namespace IO;

FileExistsDialog::FileExistsDialog(QWidget *parent) : QDialog(parent), m_overWrite(new QPushButton(this)), m_overWriteAll(new QPushButton(this)), m_skip(new QPushButton(this))
  , m_skipAll(new QPushButton(this)), m_newName(new QPushButton(this)), m_edit(new QLineEdit(this)), m_name(new QLabel(this))
{
    m_overWrite->setText(tr("Overwrite"));
    m_overWriteAll->setText(tr("Overwrite All"));
    m_skip->setText(tr("Skip"));
    m_skipAll->setText(tr("Skip All"));
    m_newName->setText(tr("New Name"));

    setWindowModality(Qt::ApplicationModal);

    connect(m_overWrite, SIGNAL(clicked()), this, SLOT(continueAction()));
    connect(m_overWriteAll, SIGNAL(clicked()), this, SLOT(continueAction()));
    connect(m_skip, SIGNAL(clicked()), this, SLOT(continueAction()));
    connect(m_skipAll, SIGNAL(clicked()), this, SLOT(continueAction()));
    connect(m_newName, SIGNAL(clicked()), this, SLOT(continueAction()));
    connect(m_edit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    QVBoxLayout *vBox = new QVBoxLayout();
    vBox->addWidget(m_name);
    vBox->addWidget(m_edit);
    QHBoxLayout *hBox = new QHBoxLayout();
    hBox->addStretch();
    hBox->addWidget(m_overWrite);
    hBox->addWidget(m_overWriteAll);
    hBox->addWidget(m_newName);
    hBox->addWidget(m_skip);
    hBox->addWidget(m_skipAll);
    hBox->addStretch();
    vBox->addLayout(hBox);
    setLayout(vBox);
    setWindowFlags( ( ( windowFlags() | Qt::CustomizeWindowHint ) & ~Qt::WindowCloseButtonHint ) );
}

void
FileExistsDialog::getNewInfo(Mode *m, QString *file)
{
    FileExistsDialog d(APP->mainWindow());
    *m = d.getMode(*file);
    if ( *m == NewName )
        *file = d.newName();
}

Mode
FileExistsDialog::getMode(const QString &file)
{
    m_mode = Cancel;
    m_file = file;
    m_overWrite->setEnabled(true);
    m_overWriteAll->setEnabled(true);
    m_newName->setEnabled(false);
    m_edit->setText(QFileInfo(file).fileName());
    m_name->setText("File " + m_file + " already exists, what do you do?");
    show();
}

void
FileExistsDialog::continueAction()
{
    if (sender() == m_overWrite)
        m_mode = Overwrite;
    else if (sender() == m_overWriteAll)
        m_mode = OverwriteAll;
    else if (sender() == m_skip)
        m_mode = Skip;
    else if (sender() == m_skipAll)
        m_mode = SkipAll;
    else if (sender() == m_newName)
        m_mode = NewName;

    m_file = QFileInfo(m_file).dir().path() + QDir::separator() +  m_edit->text();
    if ( m_mode == NewName && QFileInfo(m_file).exists() )
    {
        getMode(m_file);
        return;
    }
    emit fileExistsData(m_mode, m_file);
    hide();
}

void
FileExistsDialog::textChanged(const QString &text)
{
    const bool hasChanged = text != QFileInfo(m_file).fileName();
    m_overWrite->setEnabled( !hasChanged );
    m_overWriteAll->setEnabled( !hasChanged );
    m_newName->setEnabled( hasChanged /*&& !QFileInfo( QFileInfo(m_file).path() + QDir::separator() + text ).exists()*/ );
}

bool
FileExistsDialog::event(QEvent *event)
{
    if ( QKeyEvent *ke = dynamic_cast<QKeyEvent*>(event) )
        if ( ke->key() == Qt::Key_Escape )
            return true; //user should not can escape
    return QDialog::event(event);
}

//--------------------------------------------------------------------------------------------------------------

CopyDialog::CopyDialog(QWidget *parent) : QDialog(parent), m_ok(new QPushButton(this)), m_cancel(new QPushButton(this)), m_pause(new QPushButton(this))
  , m_fileProgress(new QProgressBar(this)), m_progress(new QProgressBar(this)), m_from(new QLabel(this)), m_inFile(new QLabel(this)), m_to(new QLabel(this))
  , m_cbHideFinished(new QCheckBox(this))
{
    m_hideFinished = Configuration::settings()->value("hideCPDWhenFinished", 0).toBool();
    m_cbHideFinished->setChecked(m_hideFinished);

    setWindowModality(Qt::ApplicationModal);

    QFont boldFont(font());
    boldFont.setBold(true);

    m_ok->setText(tr("Close"));
    m_cancel->setText(tr("Cancel"));
    m_pause->setText(tr("Pause"));
    m_ok->setEnabled(false);

    connect(m_ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_pause, SIGNAL(clicked()), this, SLOT(pauseCLicked()));

    QLabel *fr = new QLabel(tr("From:"));    //from
    fr->setFont(boldFont);
    QHBoxLayout *hlFrom = new QHBoxLayout();
    hlFrom->addWidget(fr);
    hlFrom->addStretch();
    hlFrom->addWidget(m_from);

    QLabel *toLbl = new QLabel(tr("To:"));  //to
    toLbl->setFont(boldFont);
    QHBoxLayout *hlTo = new QHBoxLayout();
    hlTo->addWidget(toLbl);
    hlTo->addStretch();
    hlTo->addWidget(m_to);

    QLabel *inLbl = new QLabel(tr("Current File:"));  //in
    inLbl->setFont(boldFont);
    QHBoxLayout *hlCf = new QHBoxLayout();
    hlCf->addWidget(inLbl);
    hlCf->addStretch();
    hlCf->addWidget(m_inFile);

    QVBoxLayout *vBoxL = new QVBoxLayout();
    vBoxL->addLayout(hlFrom);
    vBoxL->addLayout(hlTo);
    vBoxL->addSpacing(32);
    vBoxL->addStretch();
    vBoxL->addLayout(hlCf);
    vBoxL->addWidget(m_fileProgress);
    QLabel *ov = new QLabel(tr("Overall Progress:"));
    ov->setFont(boldFont);
    vBoxL->addWidget(ov);
    vBoxL->addWidget(m_progress);
    QHBoxLayout *hBoxL = new QHBoxLayout();
    hBoxL->addWidget(m_ok);
    hBoxL->addStretch();
    hBoxL->addWidget(m_pause);
    hBoxL->addWidget(m_cancel);

    connect(m_cbHideFinished, SIGNAL(toggled(bool)), this, SLOT(finishedToggled(bool)));
    m_cbHideFinished->setText(tr("Hide this dialog when operation finishes"));
    vBoxL->addWidget(m_cbHideFinished);
    vBoxL->addLayout(hBoxL);

    setLayout(vBoxL);
    setWindowFlags( ( ( windowFlags() | Qt::CustomizeWindowHint ) & ~Qt::WindowCloseButtonHint ) );
}

void
CopyDialog::hideEvent(QHideEvent *event)
{
    Configuration::settings()->setValue("hideCPDWhenFinished", m_hideFinished);
    QDialog::hideEvent(event);
}

void
CopyDialog::showEvent(QShowEvent *event)
{
    m_hideFinished = Configuration::settings()->value("hideCPDWhenFinished", 0).toBool();
    m_cbHideFinished->setChecked(m_hideFinished);
    m_ok->setEnabled(m_progress->value() == 100);
    m_cancel->setEnabled(m_progress->value() < 100);
    m_pause->setEnabled(m_progress->value() < 100);
    QDialog::showEvent(event);
}

void
CopyDialog::setInfo(QString from, QString to, int completeProgress, int currentProgress)
{
    m_progress->setValue(completeProgress);
    m_fileProgress->setValue(currentProgress);
    m_inFile->setText(QFileInfo(from).fileName());
    m_from->setText(from);
    m_to->setText(to);
    m_ok->setEnabled(m_progress->value() == 100);
    m_cancel->setEnabled(m_progress->value() < 100);
    m_pause->setEnabled(m_progress->value() < 100);
}

void
CopyDialog::finished()
{
    setWindowTitle(tr("Operation Finished"));
    m_ok->setEnabled(true);
    m_cancel->setEnabled(false);
    m_pause->setEnabled(false);
    if (m_hideFinished)
        accept();
}

void
CopyDialog::pauseCLicked()
{
    if (!m_paused)
    {
        m_pause->setText(tr("Resume"));
        m_paused = true;
    }
    else
    {
        m_pause->setText(tr("Pause"));
        m_paused = false;
    }
    emit pauseRequest(m_paused);
}

//--------------------------------------------------------------------------------------------------------------

Job *jobInstance = 0;

Job::Job(QObject *parent) : QObject(parent)
  , m_fileExistsDialog(new FileExistsDialog(APP->mainWindow()))
  , m_copyDialog(new CopyDialog(APP->mainWindow())) { }

Job
*Job::instance()
{
    if ( !jobInstance )
        jobInstance = new Job( APP->mainWindow() );
    return jobInstance;
}

void
Job::getDirs(const QString &dir, quint64 *fileSize)
{
    const QFileInfoList &entries = QDir(dir).entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    foreach(const QFileInfo &entry, entries)
    {
        QString file(entry.filePath());
        if (entry.isDir())
            getDirs(file, fileSize);
        else
            *fileSize += entry.size();
    }
}

void
Job::cp(const QStringList &copyFiles, const QString &destination, const bool &cut)
{
    m_canceled = false;
    m_cut = cut;

    m_fileSize = 0;
    m_fileProgress = 0;

    m_copyDialog->setWindowTitle(cut ? "Moving..." : "Copying...");
    m_copyDialog->setSizeGripEnabled(false);
    m_copyDialog->show();

    foreach (const QString &file, copyFiles)
    {
        QFileInfo fileInfo(file);
        if (fileInfo.isDir())
            getDirs(file, &m_fileSize);
        else
            m_fileSize += fileInfo.size();
    }

#ifdef Q_WS_X11
    if ( m_fileSize > Operations::getDriveInfo<Operations::Free>( destination ) )
    {
        QMessageBox::critical(APP->mainWindow(), tr("not enough room on destination"), QString("%1 has not enough space").arg(destination));
        m_copyDialog->hide();
        return;
    }
#endif

    IOThread *iot = new IOThread(copyFiles, destination, cut, m_fileSize, this);
    connect(m_copyDialog, SIGNAL(pauseRequest(bool)), iot, SLOT(setPaused(bool)));
    connect(m_copyDialog, SIGNAL(rejected()), iot, SLOT(cancelCopy()));
    connect(iot, SIGNAL(copyProgress(QString, QString, int, int)), m_copyDialog, SLOT(setInfo(QString, QString, int, int)));
    connect(iot, SIGNAL(finished()), m_copyDialog, SLOT(finished()));
    connect(iot, SIGNAL(fileExists(QString)), m_fileExistsDialog, SLOT(getMode(QString)));
    connect(iot, SIGNAL(pauseToggled(bool,bool)), m_copyDialog, SLOT(pauseToggled(bool, bool)));
    connect(m_fileExistsDialog, SIGNAL(fileExistsData(Mode,QString)), iot, SLOT(fileExistsAction(Mode,QString)));
    iot->start(); //start copying....
}

//---------------------------------------------------------------------------------------------------------

IOThread::IOThread(const QStringList &inf, const QString &dest, const bool &cut, const qint64 &totalSize, QObject *parent)
    : QThread(parent)
    , m_canceled(false)
    , m_overWriteAll(false)
    , m_skipAll(false)
    , m_inFiles(inf)
    , m_destDir(dest)
    , m_cut(cut)
    , m_total(totalSize)
    , m_allProgress(0)
    , m_fileProgress(0)
    , m_pause(false)
    , m_newFile(QString())
    , m_type(Copy)
{ connect(this, SIGNAL(finished()), this, SLOT(deleteLater())); }

void
IOThread::run()
{

    switch (m_type)
    {
    case Copy:
    {
        bool done = false;
        const quint64 &destId = Operations::getDriveInfo<Operations::Id>( m_destDir );
        while (!m_canceled && !done)
        {
            foreach (QString inFile, m_inFiles)
            {
                m_inFile = inFile;
                const bool &sameDisk = destId != 0 && ( (quint64)Operations::getDriveInfo<Operations::Id>( inFile ) ==  destId );
                copyRecursive( inFile, m_destDir+QDir::separator()+QFileInfo(inFile).fileName(), m_cut, sameDisk );
            }
            done = true;
        }
        break;
    }
    case Remove:
    {
        remove(m_rmPath);
        break;
    }
    }
}

void
IOThread::setPaused(bool pause)
{
    emit pauseToggled( pause, m_cut );
    m_mutex.lock();
    m_pause = pause;
    m_mutex.unlock();
    if (!m_pause)
        m_pauseCond.wakeAll();
}

bool
IOThread::copyRecursive(const QString &inFile, const QString &outFile, bool cut, bool sameDisk)
{
    if ( m_canceled )
        return false;

    bool error = false;

    if (!m_overWriteAll && !m_skipAll)
        if (QFileInfo(outFile).exists() && !QFileInfo(outFile).isDir())
        {
            emit fileExists(outFile);
            setPaused(true);
        }

    m_mutex.lock();
    if (m_pause)
        m_pauseCond.wait(&m_mutex);
    m_mutex.unlock();

    if (cut && sameDisk && !m_canceled)
        if (QFile::rename(inFile, outFile)) //if we move inside the same disk we just rename, very fast
        {
            qDebug() << "renamed with QFile::rename()";
            return true;
        }

    QString of = outFile;
    const bool &isDir = QFileInfo(of).isDir();

    if ((m_mode == Overwrite || m_overWriteAll) && !isDir)  //just remove the file and continue
        remove(of);

    if (m_mode == OverwriteAll && !isDir)
    {
        m_overWriteAll = true;
        remove(of);
    }

    if ((m_mode == Skip || m_skipAll) && !isDir)  //well, we don't want to do anything do we?
        return error;

    if (m_mode == SkipAll && !isDir)
    {
        m_skipAll = true;
        return error;
    }

    if (!m_newFile.isEmpty() && m_mode == NewName && !isDir)
    {
        of = m_newFile;
        m_newFile.clear();
    }

    if (QFileInfo(inFile).isDir())
    {
        if (!QFileInfo(of).exists())
            error |= QDir().mkdir(of);
        const QFileInfoList &entries = QDir(inFile).entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
        foreach(const QFileInfo &entry, entries)
            error |= copyRecursive( entry.filePath() , of + QDir::separator() + entry.fileName(), cut, sameDisk );
        if (cut && error && QFileInfo(of).exists())
            remove(inFile);
        return error;
    }

    emit copyProgress(inFile, of, currentProgress(), 0);
    error |= clone(inFile, of);
    m_fileProgress = 0;
    if ( !error || m_canceled )
        return false;

    m_allProgress += QFileInfo(inFile).size();

    if (cut && error && QFileInfo(of).exists())
        remove(inFile);

    return error;
}

bool
IOThread::clone(const QString &in, const QString &out)
{
    if (m_canceled)
        return false;
    if (QFileInfo(out).exists())
        return false;
    if (!QFileInfo(QFileInfo(out).absoluteDir().path()).isWritable())
        return false;
    QFile fileIn(in);
    if (!fileIn.open(QIODevice::ReadOnly))
        return false;

    QFile fileOut(out);
    fileOut.open(QIODevice::WriteOnly);

    QDataStream inData(&fileIn);
    QDataStream outData(&fileOut);

    qint64 inBytes = 0, outBytes = 0, totalInBytes = 0, totalSize = fileIn.size();
    m_fileProgress = 0;
    char block[1048576]; //read/write 1 megabyte at a time and emit progress

    int inProgress = 0;

    while (!fileIn.atEnd())
    {
        m_mutex.lock();
        if (m_pause)
            m_pauseCond.wait(&m_mutex);
        m_mutex.unlock();
        if (m_canceled)
            return false;

        inBytes = inData.readRawData(block, sizeof block);
        outBytes += outData.writeRawData(block, inBytes);
        totalInBytes += inBytes;
        m_fileProgress = outBytes;
        inProgress = int(((float)totalInBytes/totalSize)*100);
        emit copyProgress(in, out, currentProgress(), inProgress);
    }

    if (outBytes != totalInBytes)
        return false;

    fileIn.close();
    fileOut.close();
    return true;
}

bool
IOThread::remove(const QString &path) const
{
    if ( m_canceled && m_type == Copy )
        return false;
    if (!QFileInfo(path).isDir())
        QFile::remove(path);
    QDirIterator it(path, QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden , QDirIterator::Subdirectories);
    QStringList children;
    while (it.hasNext())
        children.prepend(it.next());
    children.append(path);

    bool error = false;

    for (int i = 0; i < children.count(); ++i)
    {
        QFileInfo info(children.at(i));

        if (info.isDir())
            error |= QDir().rmdir(info.filePath());
        else
            error |= QFile::remove(info.filePath());
    }
    return error;
}

void
IOThread::fileExistsAction(const Mode &m, const QString &file)
{
    m_mode = m;
    if (m == Cancel)
    {
        cancelCopy();
        return;
    }
    if (m == NewName)
    {
        if (QFileInfo(file).exists())
        {
            emit fileExists(file);
            return;
        }
        m_newFile = file;
    }
    setPaused(false);
}
