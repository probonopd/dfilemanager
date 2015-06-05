#include "helpers.h"
#include "searchbox.h"
#include "mainwindow.h"

#include <QDateTime>
#include <QSettings>
#include <QDebug>

DMimeProvider::DMimeProvider()
{
#if defined(HASMAGIC)
    m_mime = magic_open(MAGIC_MIME_TYPE);
    magic_load(m_mime, NULL);
    m_all = magic_open(MAGIC_CONTINUE);
    magic_load(m_all, NULL);
#endif
}

DMimeProvider::~DMimeProvider()
{
#if defined(HASMAGIC)
    magic_close(m_mime);
    magic_close(m_all);
#endif
}

QString
DMimeProvider::getMimeType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
#if defined(HASMAGIC)
    return magic_file(m_mime, file.toLocal8Bit().data());
#elif defined(ISWINDOWS)
    const QString &suffix = file.mid(file.lastIndexOf("."));
    if (!suffix.startsWith("."))
        return QString("Unknown");
    const QString &regCommand = QString("HKCR\\%1").arg(suffix);
    const QVariant &var = QSettings(regCommand, QSettings::NativeFormat).value("Content Type");
    if (var.isValid())
        return var.toString();
    else
        return QString("Unknown");
#endif
    return QString("Unknown");
}

QString
DMimeProvider::getFileType(const QString &file) const
{
    QMutexLocker locker(&m_mutex);
#if defined(HASMAGIC)
    return magic_file(m_all, file.toLocal8Bit().data());
#elif defined(ISWINDOWS)
    const QString &suffix = file.mid(file.lastIndexOf("."));
    if (!suffix.startsWith("."))
        return QString("Unknown");
    const QString &regCommand = QString("HKCR\\%1").arg(suffix);
    const QVariant &var = QSettings(regCommand, QSettings::NativeFormat).value(".");
    if (var.isValid())
    {
        QSettings s("HKCR", QSettings::NativeFormat);
        s.beginGroup(var.toString());
        const QString &type = s.value(".").toString();
        if (!type.isEmpty())
            return type;
        else
            return var.toString();
    }
    else
        return QString("Unknown");
#endif
    return QString("Unknown");
}

//-----------------------------------------------------------------------------

QString
DFM::DTrash::trashPath(const TrashPath &location)
{
    static QString tp[TrashNPaths];
    if (tp[location].isEmpty())
    {
        const char *xdgHomePath = getenv("XDG_DATA_HOME");
        tp[location] = QString("%1/Trash").arg(xdgHomePath?xdgHomePath:QString("%1/.local/share").arg(QDir::homePath()));
        switch (location)
        {
        case TrashFiles: tp[location].append("/files"); break;
        case TrashInfo: tp[location].append("/info"); break;
        default: break;
        }
    }
    return tp[location];
}

QString
DFM::DTrash::trashFilePath(QString file)
{
    file.remove(trashPath(TrashFiles));
    QStringList path(file.split("/", QString::SkipEmptyParts));
    if (path.isEmpty())
        return QString();
    QString trashUrl = "trash:";
    while (!path.isEmpty())
        trashUrl.append(QString("/%1").arg(path.takeFirst()));
    return trashUrl;
}

static QString trashInfo(const QString &file)
{
    return QString("[Trash Info]\nPath=%1\nDeletionDate=%2\n").arg(file, QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
}

bool
DFM::DTrash::moveToTrash(const QStringList &files)
{
    qDebug() << "found trash dir at:" << trashPath();
    for (int i = 0; i < files.count(); ++i)
    {
        const QFileInfo file(files.at(i));
        if (file.absoluteFilePath().startsWith(trashPath()))
            continue; //file already in trash...
        if (!file.exists())
        {
            qDebug() << "cant move file" << file.absoluteFilePath() << "to trash. File doesnt exist.";
            continue;
        }
        const QString &ti = trashInfo(file.absoluteFilePath());
        const QString &trashInfoPath = QString("%1/%2.trashinfo").arg(trashPath(TrashInfo), file.fileName());
        const QString &trashFilePath = QString("%1/%2").arg(trashPath(TrashFiles), file.fileName());
        if (QFile::rename(file.absoluteFilePath(), trashFilePath))
        {
            QFile trashInfoFile(trashInfoPath);
            if (trashInfoFile.open(QFile::WriteOnly|QFile::Text|QFile::Truncate))
            {
                QTextStream data(&trashInfoFile);
                data.setCodec("UTF-8");
                data << ti;
                trashInfoFile.close();
            }
            else
            {
                qDebug() << "something went creating trashinfo file, aborting.";
                return false;
            }
        }
    }
    return true;
}

QString
DFM::DTrash::restorePath(QString file)
{
    file.remove(trashPath(TrashFiles));
    QStringList path(file.split("/", QString::SkipEmptyParts));
    if (path.isEmpty())
        return QString();
    QSettings settings(QString("%1/%2.trashinfo").arg(trashPath(TrashInfo), path.takeFirst()), QSettings::IniFormat);
    QString rp = settings.value("Trash Info/Path", QString()).toString();
    while (!path.isEmpty())
        rp.append(QString("/%1").arg(path.takeFirst()));
    return rp;
}

QString
DFM::DTrash::trashInfoFile(QString file)
{
    file.remove(trashPath(TrashFiles));
    QStringList path(file.split("/", QString::SkipEmptyParts));
    if (path.isEmpty())
        return QString();
    return QString("%1/%2.trashinfo").arg(trashPath(TrashInfo), path.first());
}

bool
DFM::DTrash::restoreFromTrash(const QStringList &files)
{
    for (int i = 0; i < files.count(); ++i)
    {
        const QFileInfo file(files.at(i));
        const QString restoreFile = restorePath(file.absoluteFilePath());
        const QDir restoreDir = QFileInfo(restoreFile).dir();
        if (!restoreDir.exists())
        {
            qDebug() << "the directory"
                     << restoreDir.path()
                     << "doesnt exist anymore\n"
                     << "so I cant restore there\n"
                     << "create the directory and try again.";
            continue;
        }
        if (QFile::rename(file.absoluteFilePath(), restoreFile))
        {
            const QString &tif = trashInfoFile(file.absoluteFilePath());
            if (QFileInfo(tif).exists())
                QFile::remove(tif);
        }
        else
        {
            qDebug() << "unable to restore file" << restoreFile << "aborting.";
            return false;
        }
    }
    return true;
}

bool
DFM::DTrash::removeFromTrash(const QStringList &files)
{
    for (int i = 0; i < files.count(); ++i)
    {
        const QFileInfo file(files.at(i));
        if (QFile::remove(file.absoluteFilePath()))
        {
            const QString &tif = trashInfoFile(file.absoluteFilePath());
            if (QFileInfo(tif).exists())
                QFile::remove(tif);
        }
        else
        {
            qDebug() << "unable to remove file" << trashFilePath(file.absoluteFilePath()) << "aborting.";
            return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------

void
DFM::DViewBase::keyPressEvent(QKeyEvent *ke)
{
    if (DFM::MainWindow *mw = DFM::MainWindow::currentWindow())
    {
        SearchBox *searchBox = mw->searchBox();
        if (!ke->text().isEmpty() && !ke->modifiers() && ke->key() != Qt::Key_Escape)
        {
            searchBox->setMode(Filter);
            searchBox->setText(QString("%1%2").arg(searchBox->text(), ke->text()));
            searchBox->setFocus();
        }
    }
}
