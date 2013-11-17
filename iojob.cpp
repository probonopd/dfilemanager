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
#include <QMessageBox>

#include "iojob.h"
#include "operations.h"

#include "mainwindow.h"

using namespace DFM;
using namespace IO;

FileExistsDialog::FileExistsDialog( const QStringList &files, QWidget *parent)
    : QDialog(parent)
    , m_overWrite(new QPushButton(this))
    , m_overWriteAll(new QPushButton(this))
    , m_skip(new QPushButton(this))
    , m_skipAll(new QPushButton(this))
    , m_newName(new QPushButton(this))
    , m_edit(new QLineEdit(this))
    , m_name(new QLabel(this))
    , m_file(files[1])
{
    m_overWrite->setText(tr("Overwrite"));
    m_overWriteAll->setText(tr("Overwrite All"));
    m_skip->setText(tr("Skip"));
    m_skipAll->setText(tr("Skip All"));
    m_newName->setText(tr("New Name"));
    m_edit->setText(QFileInfo(m_file).fileName());
    m_name->setText("File " + m_file + " already exists, what do you do?");

    QFileInfo inInfo(files[0]);
    QFileInfo outInfo(files[1]);

    m_overWrite->setEnabled( !(inInfo.path() == outInfo.path() && inInfo.isDir()) );
    m_overWriteAll->setEnabled( !(inInfo.path() == outInfo.path() && inInfo.isDir()) );
    m_newName->setEnabled(false);

    connect(m_overWrite, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_overWriteAll, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_skip, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_skipAll, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_newName, SIGNAL(clicked()), this, SLOT(accept()));
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
    setWindowModality(Qt::WindowModal);
}

QPair<Mode, QString> FileExistsDialog::mode(const QStringList &files) { return FileExistsDialog(files, MainWindow::currentWindow()).getMode(); }

void
FileExistsDialog::accept()
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
    QDialog::accept();
}

QPair<Mode, QString>
FileExistsDialog::getMode()
{
    m_mode = Cancel;
    exec();
    return QPair<Mode, QString>(m_mode, m_newFileName);
}

void
FileExistsDialog::textChanged(const QString &text)
{
    const bool hasChanged = text != QFileInfo(m_file).fileName();
    m_overWrite->setEnabled( !hasChanged );
    m_overWriteAll->setEnabled( !hasChanged );
    m_newFileName = QFileInfo(m_file).dir().absoluteFilePath(text);
    if ( QFileInfo(m_newFileName).exists() )
        m_name->setText("File " + m_newFileName + " already exists, what do you do?");
    m_newName->setEnabled( hasChanged && !QFileInfo(m_newFileName).exists() );
}

bool
FileExistsDialog::event(QEvent *e)
{
    if ( QKeyEvent *ke = dynamic_cast<QKeyEvent*>(e) )
        if ( ke->key() == Qt::Key_Escape )
            return true; //user should not can escape
    return QDialog::event(e);
}

//--------------------------------------------------------------------------------------------------------------

CopyDialog::CopyDialog(QWidget *parent)
    : QDialog(parent)
    , m_ok(new QPushButton(this))
    , m_cancel(new QPushButton(this))
    , m_pause(new QPushButton(this))
    , m_fileProgress(new QProgressBar(this))
    , m_progress(new QProgressBar(this))
    , m_from(new QLabel(this))
    , m_inFile(new QLabel(this))
    , m_to(new QLabel(this))
    , m_cbHideFinished(new QCheckBox(this))
    , m_speedLabel(new QLabel(this))
{
    m_hideFinished = Store::settings()->value("hideCPDWhenFinished", 0).toBool();
    m_cbHideFinished->setChecked(m_hideFinished);

    setWindowModality(Qt::WindowModal);

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

    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel(tr("Speed (Approximately):"), this));
    speedLayout->addStretch();
    speedLayout->addWidget(m_speedLabel);
    vBoxL->addLayout(speedLayout);

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
//    vBoxL->setSizeConstraint(QLayout::SetFixedSize);
    setWindowFlags( ( ( windowFlags() | Qt::CustomizeWindowHint ) & ~Qt::WindowCloseButtonHint ) );
}

static QString elidedText( const QString &text ) { return qApp->fontMetrics().elidedText(text, Qt::ElideMiddle, 256); }

void
CopyDialog::setInfo(QString from, QString to, int completeProgress, int currentProgress)
{
    m_progress->setValue(completeProgress);
    m_fileProgress->setValue(currentProgress);
    m_inFile->setText(elidedText(QFileInfo(from).fileName()));
    m_from->setText(elidedText(from));
    m_to->setText(elidedText(to));
    m_ok->setEnabled(m_progress->value() == 100);
    if ( m_progress->value() == 100 )
    {
        m_cancel->setEnabled(false);
        m_pause->setEnabled(false);
    }
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

void
CopyDialog::finishedToggled(bool enabled)
{
    m_hideFinished = enabled;
    Store::settings()->setValue("hideCPDWhenFinished", m_hideFinished);
}

//--------------------------------------------------------------------------------------------------------------    

void
Job::remove(const QStringList &paths)
{
    (new Job(MainWindow::currentWindow()))->rm(paths);
}

void
Job::copy(const QStringList &sourceFiles, const QString &destination, bool cut, bool ask)
{
    (new Job(MainWindow::currentWindow()))->cp(sourceFiles, destination, cut, ask);
}

void
Job::copy(const QList<QUrl> &sourceFiles, const QString &destination, bool cut, bool ask)
{
    (new Job(MainWindow::currentWindow()))->cp(sourceFiles, destination, cut, ask);
}

void
Job::getDirs(const QString &dir, quint64 *fileSize)
{
    const QFileInfoList &entries = QDir(dir).entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
    foreach (const QFileInfo &entry, entries)
    {
        if (entry.isDir())
            getDirs(entry.filePath(), fileSize);
        else
            *fileSize += entry.size();
    }
}

void
Job::cp(const QStringList &copyFiles, const QString &destination, bool cut, bool ask)
{
    if ( ask )
    {
        if ( QApplication::overrideCursor() )
            QApplication::restoreOverrideCursor();
        QString title(tr("Are you sure?"));
        QString message(tr("Do you want to move:<br>"));
        foreach (const QString &file, copyFiles)
            message.append("<br> - ").append(file);
        message.append(tr("<br><br>-> ")).append(destination).append(" ?");

        if ( QMessageBox::question(MainWindow::currentWindow(), title, message, QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
            return;
    }
    quint64 fileSize = 0;
    const quint64 destId = Ops::getDriveInfo<Ops::Id>( destination );
    bool sameDisk = destId != 0;
    foreach (const QString &file, copyFiles)
    {
        if ( QFileInfo(file).isDir() )
            if ( destination.startsWith(file)
                 || ( QFileInfo(file).path() == destination && cut ) )
                return;

        QFileInfo fileInfo(file);
        if (fileInfo.isDir())
            getDirs(file, &fileSize);
        else
            fileSize += fileInfo.size();

        if ( sameDisk )
            sameDisk = ( (quint64)Ops::getDriveInfo<Ops::Id>( file ) ==  destId );
    }

    CopyDialog *copyDialog = new CopyDialog(MainWindow::currentWindow());
    copyDialog->setWindowTitle(cut ? "Moving..." : "Copying...");
    copyDialog->setSizeGripEnabled(false);
    copyDialog->show();

#ifdef Q_WS_X11
    if ( fileSize > Ops::getDriveInfo<Ops::Free>( destination ) && !(cut && sameDisk) )
    {
        QMessageBox::critical(MainWindow::currentWindow(), tr("not enough room on destination"), QString("%1 has not enough space").arg(destination));
        copyDialog->reject();
        return;
    }
#endif

    IOThread *ioThread = new IOThread(copyFiles, destination, cut, fileSize, this);
    connect(copyDialog, SIGNAL(pauseRequest(bool)), ioThread, SLOT(setPaused(bool)));
    connect(copyDialog, SIGNAL(rejected()), ioThread, SLOT(cancelCopy()));
    connect(ioThread, SIGNAL(copyProgress(QString, QString, int, int)), copyDialog, SLOT(setInfo(QString, QString, int, int)));
    connect(ioThread, SIGNAL(finished()), copyDialog, SLOT(finished()));
    connect(ioThread, SIGNAL(pauseToggled(bool,bool)), copyDialog, SLOT(pauseToggled(bool, bool)));
    connect(ioThread, SIGNAL(speed(QString)), copyDialog, SLOT(setSpeed(QString)));
    ioThread->start(); //start copying....
}

void
Job::cp(const QList<QUrl> &copyFiles, const QString &destination, bool cut, bool ask)
{
    QStringList files;
    foreach ( const QUrl &url, copyFiles )
        files << url.toLocalFile();
    cp(files, destination, cut, ask);
}

//---------------------------------------------------------------------------------------------------------

IOThread::IOThread(const QStringList &inf, const QString &dest, const bool cut, const quint64 totalSize, QObject *parent)
    : QThread(parent)
    , m_canceled(false)
    , m_inFiles(inf)
    , m_destDir(dest)
    , m_cut(cut)
    , m_total(totalSize)
    , m_allProgress(0)
    , m_inProgress(0)
    , m_pause(false)
    , m_newFile(QString())
    , m_type(Copy)
    , m_outFile(QString())
    , m_fileProgress(0)
    , m_diffCheck(0)
    , m_timer(new QTimer(this))
    , m_speedTimer(new QTimer(this))
    , m_mode(Continue)
    , m_inSize(0)
{
    connect(this, SIGNAL(finished()), this, SLOT(finishedSlot()));
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(fileExists(QStringList)), this, SLOT(fileExistsSlot(QStringList)));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(emitProgress()));
    connect(this, SIGNAL(finished()), m_timer, SLOT(stop()));
    connect(this, SIGNAL(errorSignal()), this, SLOT(errorSlot()));
    connect(m_speedTimer, SIGNAL(timeout()), this, SLOT(checkSpeed()));
    m_timer->start(100);
    m_speedTimer->start(1000);
}

IOThread::IOThread(const QStringList &paths, QObject *parent)
    : QThread(parent)
    , m_type(Remove)
    , m_rmPaths(paths)
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void
IOThread::errorSlot()
{
    const QString &title = tr("Something went wrong...");
    const QString &text = QString("There was an error while copying <br><br>%1 <br><br>to<br><br>%2 <br><br>Are you sure you have write permissions in %3 ? <br><br>I will now exit.").arg(m_inFile, m_outFile, m_destDir);
    QMessageBox::critical(MainWindow::currentWindow(), title, text);
    cancelCopy();
}

void
IOThread::checkSpeed()
{
    const quint64 diff = m_allProgress-m_diffCheck;
    m_diffCheck = m_allProgress;
    emit speed(QString("%1 /s").arg(Ops::prettySize(diff)));
}

void
IOThread::emitProgress()
{
    emit copyProgress(m_inFile, m_outFile, currentProgress(), m_fileProgress);
}

void
IOThread::run()
{
    switch (m_type)
    {
    case Copy:
    {
        const quint64 destId = Ops::getDriveInfo<Ops::Id>( m_destDir );
        while (!m_canceled && !m_inFiles.isEmpty())
        {
            m_inFile = m_inFiles.takeFirst();
            const bool sameDisk = destId != 0 && ( (quint64)Ops::getDriveInfo<Ops::Id>( m_inFile ) ==  destId );
            const QString &outFile = QDir(m_destDir).absoluteFilePath(QFileInfo(m_inFile).fileName());
            copyRecursive( m_inFile, outFile, m_cut, sameDisk );
        }
        break;
    }
    case Remove:
    {
        while ( !m_rmPaths.isEmpty() )
            remove(m_rmPaths.takeFirst());
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

void
IOThread::setMode(QPair<Mode, QString> mode)
{
    m_mode = mode.first;
    m_newFile = m_mode == NewName ? mode.second : QString();
    if ( m_mode == Cancel || m_mode == SkipAll )
        cancelCopy();
    else
        setPaused(false);
}

#define PAUSE m_mutex.lock(); \
    if (m_pause) \
        m_pauseCond.wait(&m_mutex); \
    m_mutex.unlock() \

void
IOThread::copyRecursive(const QString &inFile, const QString &outFile, bool cut, bool sameDisk)
{
    if ( m_canceled )
        return;

    if ( m_mode != OverwriteAll )
        if (QFileInfo(outFile).exists())
        {
            emit fileExists(QStringList() << inFile << outFile);
            setPaused(true);
        }

    PAUSE;

    if ( m_mode == Overwrite || m_mode == OverwriteAll )
    {
        if ( !QFileInfo(outFile).isDir() )
            remove(outFile);
        if ( m_mode == Overwrite )
            m_mode = Continue;
    }
    else if (m_mode == Skip)
    {
        m_mode = Continue;
        return;
    }

    if (m_mode == NewName && !m_newFile.isEmpty())
    {
        m_mode = Continue;
        copyRecursive(inFile, m_newFile, cut, sameDisk);
        m_newFile = QString();
        return;
    }

    if (cut && sameDisk && !m_canceled)
    {
        QFile::rename(inFile, outFile); //if we move inside the same disk we just rename, very fast
        return;
    }

    if (QFileInfo(inFile).isDir())
    {
        if (!QFileInfo(outFile).exists())
            if (!QDir().mkpath(outFile))
            {
                m_inFile = inFile;
                m_outFile = outFile;
                emit errorSignal();
                setPaused(true);
                PAUSE;
            }
        const QDir &inDir(inFile);
        const QStringList &entries(inDir.entryList(QDir::AllDirs|QDir::Files|QDir::NoDotAndDotDot|QDir::Hidden, QDir::Name|QDir::DirsFirst));
        const int n = entries.count();
        for ( int i = 0; i < n; ++i )
        {
            const QString &name = entries.at(i);
            copyRecursive( inDir.absoluteFilePath(name) , QDir(outFile).absoluteFilePath(name), cut, sameDisk );
        }
    }
    else if ( !clone(inFile, outFile) )
    {
        emit errorSignal();
        setPaused(true);
        PAUSE;
    }

    if (cut && QFileInfo(outFile).exists())
        remove(inFile);
}

bool
IOThread::clone(const QString &in, const QString &out)
{
    m_inFile = in;
    m_outFile = out;
    if (m_canceled)
        return true;
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

    quint64 inBytes = 0, totalInBytes = 0, totalSize = fileIn.size();
    m_fileProgress = m_inProgress = 0;
    char block[1048576]; //read/write 1 megabyte at a time
    m_inSize = fileIn.size();

    while (!fileIn.atEnd())
    {
        PAUSE;
        if (m_canceled)
        {
            fileIn.close();
            fileOut.close();
            QFile::remove(out);
            return true;
        }
        inBytes = inData.readRawData(block, sizeof block);
        m_inProgress += outData.writeRawData(block, inBytes);
        m_allProgress += inBytes;
        totalInBytes += inBytes;
        m_fileProgress = totalInBytes*100/totalSize;
    }

    fileIn.close();
    fileOut.close();

    return m_inProgress == totalInBytes;
}

#undef PAUSE

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
