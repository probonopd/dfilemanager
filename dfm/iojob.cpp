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
#include <QLocalSocket>
#include <QProcess>

#include "iojob.h"
#include "operations.h"
#include "mainwindow.h"
#include "application.h"
#include "config.h"

using namespace DFM;
using namespace IO;

static QString s_errorBase = QString("There was an error while copying <br><br>%1"
                                    "<br><br>to<br><br>%2 <br><br>Are you sure you"
                                    "have write permissions in %3 ? <br><br>I will now exit.");

FileExistsDialog::FileExistsDialog(const QStringList &files, QWidget *parent)
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

    m_overWrite->setEnabled(!(inInfo.path() == outInfo.path() && inInfo.isDir()));
    m_overWriteAll->setEnabled(!(inInfo.path() == outInfo.path() && inInfo.isDir()));
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
    setWindowFlags(((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint));
    setWindowModality(Qt::WindowModal);
}

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

Mode
FileExistsDialog::getMode()
{
    m_mode = Cancel;
    exec();
    return m_mode;
}

void
FileExistsDialog::textChanged(const QString &text)
{
    const bool hasChanged = text != QFileInfo(m_file).fileName();
    m_overWrite->setEnabled(!hasChanged);
    m_overWriteAll->setEnabled(!hasChanged);
    m_newFileName = QFileInfo(m_file).dir().absoluteFilePath(text);
    if (QFileInfo(m_newFileName).exists())
        m_name->setText("File " + m_newFileName + " already exists, what do you do?");
    m_newName->setEnabled(hasChanged && !QFileInfo(m_newFileName).exists());
}

bool
FileExistsDialog::event(QEvent *e)
{
    if (QKeyEvent *ke = dynamic_cast<QKeyEvent*>(e))
        if (ke->key() == Qt::Key_Escape)
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
    , m_cut(false)
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
    setWindowFlags(((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint));
}

static QString elidedText(const QString &text) { return qApp->fontMetrics().elidedText(text, Qt::ElideMiddle, 256); }

void
CopyDialog::setInfo(QString from, QString to, int completeProgress, int currentProgress)
{
    m_progress->setValue(completeProgress);
    m_fileProgress->setValue(currentProgress);
    m_inFile->setText(elidedText(QFileInfo(from).fileName()));
    m_from->setText(elidedText(from));
    m_to->setText(elidedText(to));
    const bool isFinished = m_progress->value() == 100;
    m_ok->setEnabled(isFinished);
    m_pause->setEnabled(!isFinished);
    m_cancel->setEnabled(!isFinished);
    if (m_progress->value() == 100)
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

void
CopyDialog::pauseToggled(const bool pause)
{
    setWindowTitle(m_cut ? (pause ? "Moving... [paused]" : "Moving...") : (pause ? "Copying... [paused]" : "Copying... "));
}


//--------------------------------------------------------------------------------------------------------------

void
Manager::remove(const QStringList &paths)
{
    IOJobData ioJobData;
    QString inPaths;
    for (int i = 0; i < paths.count(); ++i)
    {
        inPaths.append(paths.at(i));
        if (i != paths.count()-1)
            inPaths.append(SEP);
    }
    ioJobData.inPaths = inPaths;
    ioJobData.outPath = QString();
    ioJobData.ioTask = RemoveTask;

    QProcess::startDetached(dApp->applicationFilePath(), Ops::fromIoJobData(ioJobData));
}

void
Manager::copy(const QStringList &sourceFiles, const QString &destination, bool cut, bool ask)
{
    IOJobData ioJobData;
    QString inPaths;
    for (int i = 0; i < sourceFiles.count(); ++i)
    {
        inPaths.append(sourceFiles.at(i));
        if (i != sourceFiles.count()-1)
            inPaths.append(SEP);
    }
    ioJobData.inPaths = inPaths;
    ioJobData.outPath = destination;
    ioJobData.ioTask = cut?MoveTask:CopyTask;

    QProcess::startDetached(qApp->applicationFilePath(), Ops::fromIoJobData(ioJobData));
}

void
Manager::copy(const QList<QUrl> &sourceFiles, const QString &destination, bool cut, bool ask)
{
    QStringList files;
    foreach (const QUrl &url, sourceFiles)
    {
        const QFileInfo &i = url.toLocalFile();
        if (i.exists())
            files << i.filePath();
    }
    if (!files.isEmpty())
        copy(files, destination, cut, ask);
}

Manager::Manager(QObject *parent)
    : DThread(parent)
    , m_canceled(false)
    , m_cut(false)
    , m_total(0) //size in bytes of all files we are going to copy/move
    , m_allProgress(0) //overall progress
    , m_inProgress(0) //current file progress
    , m_fileProgress(0) //written bytes to the file we are currently working on, also used to check if clone was succesful
    , m_diffCheck(0) //used to calculate speed
    , m_destDir(QString()) //destination
    , m_newFile(QString()) //new name of file when file exists
    , m_outFile(QString()) //file we are currently copying to
    , m_timer(new QTimer(this))
    , m_speedTimer(new QTimer(this))
    , m_mode(Continue)
    , m_copyDialog(new CopyDialog())
{
    m_copyDialog->setSizeGripEnabled(false);
    m_timer->setInterval(100);
    m_speedTimer->setInterval(1000);

    connect(m_copyDialog, SIGNAL(pauseRequest(bool)), this, SLOT(setPause(bool)));
    connect(m_copyDialog, SIGNAL(rejected()), this, SLOT(cancelCopy()));

    connect(this, SIGNAL(copyProgress(QString, QString, int, int)), m_copyDialog, SLOT(setInfo(QString, QString, int, int)));
    connect(this, SIGNAL(finished()), m_copyDialog, SLOT(finished()));
    connect(this, SIGNAL(pauseToggled(bool)), m_copyDialog, SLOT(pauseToggled(bool)));
    connect(this, SIGNAL(speed(QString)), m_copyDialog, SLOT(setSpeed(QString)));
    connect(this, SIGNAL(copyOrMoveStarted()), m_copyDialog, SLOT(show()));
    connect(this, SIGNAL(isMove(bool)), m_copyDialog, SLOT(setMove(bool)));

    connect(this, SIGNAL(finished()), this, SLOT(finishedSlot()));
    connect(this, SIGNAL(fileExists(QStringList)), this, SLOT(fileExistsSlot(QStringList)));
    connect(this, SIGNAL(errorSignal()), this, SLOT(errorSlot()));

    connect(m_timer, SIGNAL(timeout()), this, SLOT(emitProgress()));
    connect(this, SIGNAL(finished()), m_timer, SLOT(stop()));

    connect(m_speedTimer, SIGNAL(timeout()), this, SLOT(checkSpeed()));

    connect(this, SIGNAL(copyOrMoveStarted()), m_timer, SLOT(start()));
    connect(this, SIGNAL(copyOrMoveStarted()), m_speedTimer, SLOT(start()));

    connect(this, SIGNAL(copyOrMoveFinished()), m_timer, SLOT(stop()));
    connect(this, SIGNAL(copyOrMoveFinished()), m_speedTimer, SLOT(stop()));

    connect(this, SIGNAL(ioIsBusy(bool)), this, SLOT(ioBusy(bool)));
}

Manager::~Manager()
{
//    APP->setMessage(QStringList() << "--status" << "destroying IO manager", "dfm_browser");
    delete m_copyDialog;
}

void
Manager::finishedSlot()
{
    emit copyProgress(QString(), QString(), 100, 100); emit speed(QString());
    if (DFM::Store::config.behaviour.useIOQueue)
        dApp->setMessage(QStringList() << "--ioProgress" << "100", "dfm_browser");
    if (Store::settings()->value("hideCPDWhenFinished").toBool())
        m_copyDialog->hide();
}

void
Manager::doJob(const IOJobData &ioJobData)
{
    reset();
    m_currentJob = ioJobData;
//    qDebug() << "doing job:";
//    qDebug() << Ops::taskToString(ioJobData.ioTask) << ioJobData.inList;
    if (ioJobData.ioTask < RemoveTask)
    {
        m_cut = bool(ioJobData.ioTask==MoveTask);
        emit isMove(m_cut);
        m_destDir = ioJobData.outPath;
        m_total = 0;
        const QStringList copyFiles = ioJobData.inList;
        if (!getTotalSize(copyFiles, m_total))
            return;

        emit copyOrMoveStarted();
        foreach (const QString &file, copyFiles)
        {
            if (m_canceled)
                break;
            m_inFile = file;
            m_outFile = QDir(m_destDir).absoluteFilePath(QFileInfo(file).fileName());
            const bool sameDisk = Ops::sameDisk(file, m_destDir);
            const bool canRename = bool(m_cut && sameDisk);

            if (canRename)
            {
                if (!QFile::rename(m_inFile, m_outFile))
                    error(s_errorBase.arg(m_inFile, m_outFile, m_destDir));
                continue;
            }
#if defined(HASSYS)
            if (m_total > Ops::getDriveInfo<Ops::Free>(m_destDir))
                error(QString("Not enough space on %1").arg(m_destDir));
#endif
            if (!copyRecursive(file, m_outFile, m_cut, sameDisk))
                error(s_errorBase.arg(m_inFile, m_outFile, m_destDir));
            else if (m_cut)
                remove(file);
        }
        emit copyOrMoveFinished();
    }
    else if (ioJobData.ioTask == RemoveTask)
    {
        emit ioIsBusy(true);
        foreach (const QString &file, ioJobData.inList)
            remove(file);
        emit ioIsBusy(false);
    }
}

void
Manager::ioBusy(const bool busy)
{
    if (!DFM::Store::config.behaviour.useIOQueue)
        return;
    QString i = busy?"-1":"100";
    QStringList message = QStringList() << "--ioProgress" << i;
    if (busy)
        message << "Deleting...";
    dApp->setMessage(message, "dfm_browser");
}

void
Manager::reset()
{
    m_canceled = false;
    m_cut = false;
    m_total = 0; //size in bytes of all files we are going to copy/move
    m_allProgress = 0; //overall progress
    m_inProgress = 0; //current file progress
    m_fileProgress = 0; //written bytes to the file we are currently working on, also used to check if clone was succesful
    m_diffCheck = 0; //used to calculate speed
    m_destDir = QString(); //destination
    m_newFile = QString(); //new name of file when file exists
    m_outFile = QString(); //file we are currently copying to
    m_errorString = QString();
    m_mode = Continue;
}

void
Manager::error(const QString &error)
{
    m_errorString = error;
    emit errorSignal();
    setPause(true);
    pause();
}

void
Manager::getDirs(const QString &dir, quint64 &fileSize)
{
    const QFileInfoList &entries = QDir(dir).entryInfoList(allEntries);
    foreach (const QFileInfo &entry, entries)
    {
        if (entry.isDir())
            getDirs(entry.filePath(), fileSize);
        else
            fileSize += entry.size();
    }
}

bool
Manager::getTotalSize(const QStringList &copyFiles, quint64 &fileSize)
{
    foreach (const QString &file, copyFiles)
    {
        if (QFileInfo(file).isDir())
            if (m_destDir.startsWith(file) || (QFileInfo(file).path() == m_destDir && m_cut))
                return false;

        QFileInfo fileInfo(file);
        if (fileInfo.isDir())
            getDirs(file, fileSize);
        else
            fileSize += fileInfo.size();
    }
    return true;
}

void
Manager::getMessage(const QStringList &message)
{
    IOJobData ioJobData;
    if (Ops::extractIoData(message, ioJobData))
        queue(ioJobData);
}

void
Manager::errorSlot()
{
    const QString &title = tr("Something went wrong...");
    QMessageBox::critical(MainWindow::currentWindow(), title, m_errorString);
    cancelCopy();
}

void
Manager::checkSpeed()
{
    const quint64 diff = m_allProgress-m_diffCheck;
    m_diffCheck = m_allProgress;
    emit speed(QString("%1 /s").arg(Ops::prettySize(diff)));
}

void
Manager::emitProgress()
{
    const int progress = currentProgress();
    emit copyProgress(m_inFile, m_outFile, progress, m_fileProgress);
    if (!DFM::Store::config.behaviour.useIOQueue)
        return;
    QString task = m_cut?"Moving...":"Copying...";
//    task.append(QString("\n%1 %2 %3").arg("queue: ", QString::number(queueCount()), " items"));
    dApp->setMessage(QStringList() << "--ioProgress" << QString::number(progress) << task, "dfm_browser");
}

void
Manager::run()
{
    while (hasQueue())
    {
        m_canceled=false;
        doJob(deqeueue());
    }
}

void
Manager::queue(const IOJobData &ioJob)
{
//    qDebug() << "got new iojob" << ioJob.ioTask << ioJob.inList;
    m_queueMtx.lock();
    m_queue << ioJob;
    m_queueMtx.unlock();
    start();
}

IOJobData
Manager::deqeueue()
{
    QMutexLocker locker(&m_queueMtx);
    return m_queue.dequeue();
}

bool
Manager::hasQueue() const
{
    QMutexLocker locker(&m_queueMtx);
    return !m_queue.isEmpty();
}

int
Manager::queueCount() const
{
    QMutexLocker locker(&m_queueMtx);
    return m_queue.count();
}

void
Manager::fileExistsSlot(const QStringList &files)
{
    FileExistsDialog d(files);
    Mode m = d.getMode();
    if (m == NewName)
        m_newFile = d.newName();

    setMode(m);
}

void
Manager::setMode(Mode mode)
{
    m_mode = mode;
    if (m_mode == Cancel)
        cancelCopy();
    else
        setPause(false);
}

bool
Manager::copyRecursive(const QString &inFile, const QString &outFile, bool cut, bool sameDisk)
{
    if (m_canceled)
        return true;

    const QFileInfo outFileInfo(outFile);

    if (m_mode != OverwriteAll)
    {
        if (outFileInfo.exists())
        {
            if (m_mode == SkipAll)
                return true;
            emit fileExists(QStringList() << inFile << outFile);
            setPause(true);
        }
    }

    pause();

    if (m_mode == SkipAll && outFileInfo.exists())
        return true;

    if (m_mode == Overwrite || m_mode == OverwriteAll)
    {
        if (!QFileInfo(outFile).isDir())
            remove(outFile);
        if (m_mode == Overwrite)
            m_mode = Continue;
    }
    else if (m_mode == Skip)
    {
        m_mode = Continue;
        return true;
    }

    if (m_mode == NewName && !m_newFile.isEmpty())
    {
        m_mode = Continue;
        if (!copyRecursive(inFile, m_newFile, cut, sameDisk))
        {
            m_newFile = QString();
            return false;
        }
        else return true;
    }

    if (!clone(inFile, outFile))
        return false;

    if (QFileInfo(inFile).isDir())
    {
        QDir inDir(inFile), outDir(outFile);
        inDir.setFilter(allEntries);
        QDirIterator dirIterator(inDir);
        while (dirIterator.hasNext())
        {
            const QString &name = QFileInfo(dirIterator.next()).fileName();
            if (!copyRecursive(inDir.absoluteFilePath(name) , outDir.absoluteFilePath(name), cut, sameDisk))
                return false;
        }
    }
    return true;
}

bool
Manager::clone(const QString &in, const QString &out)
{
    m_inFile = in;
    m_outFile = out;
    if (m_canceled)
        return true;
    if (QFileInfo(in).isDir())
        return QDir(out).mkpath(out);

    QFileInfo outInfo(out);
    if (outInfo.exists())
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

    while (!fileIn.atEnd())
    {
        pause();
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

    return bool(m_inProgress == totalInBytes);
}

bool
Manager::remove(const QString &path) const
{
    if (m_canceled)
        return false;
    if (!QFileInfo(path).isDir())
        return QFile::remove(path);
    QDirIterator it(path, allEntries, QDirIterator::Subdirectories);
    QStringList children;
    while (it.hasNext())
        children.prepend(it.next());
    children.append(path);

    bool error = false;

    for (int i = 0; i < children.count(); ++i)
    {
        const QFileInfo info(children.at(i));

        if (info.isDir())
            error |= QDir().rmdir(info.filePath());
        else
            error |= QFile::remove(info.filePath());
    }
    return error;
}
