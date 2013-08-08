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


#include "filesystemmodel.h"
#include "iojob.h"
#include "application.h"
#include <QImageReader>
#include <QMessageBox>
#include "thumbsloader.h"
#include "viewcontainer.h"
#include "mainwindow.h"


using namespace DFM;

FileSystemModel::FileSystemModel(QObject *parent) :
    QFileSystemModel(parent)
{
    setResolveSymlinks(false);
    setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
    setNameFilterDisables(false);
    setReadOnly(false);
    m_parent = parent;
    m_nameThumbs = supportedThumbs();
    connect ( this, SIGNAL(rootPathChanged(QString)), this, SLOT(emitRootIndex(QString)) );
}

bool
FileSystemModel::remove(const QModelIndex &myIndex) const
{
    QString path = filePath(myIndex);

    QDirIterator it(path,QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden ,QDirIterator::Subdirectories);
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

QPixmap
FileSystemModel::iconPix(const QFileInfo &info, const int &extent)
{
    const QSettings settings(info.filePath() + QDir::separator() + ".directory", QSettings::IniFormat);
    QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString());
    if ( icon.isNull() )
        icon = QIcon::fromTheme("inode-directory");
    if ( icon.isNull() )
        return QPixmap();

    int actSize = extent;
    while ( !icon.availableSizes().contains(QSize(actSize, actSize)) )
        ++actSize;

    return icon.pixmap(actSize);
}

QVariant
FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this)
        return QVariant();

    if (index.column() == 3 && role == Qt::DisplayRole)
    {
        QString lastMod;
        const QDateTime lastDate = lastModified(index);
        lastMod.append(QString::number(lastDate.date().day()) + "/");
        lastMod.append(QString::number(lastDate.date().month()) + "/");
        lastMod.append(QString::number(lastDate.date().year()) + " ");
        lastMod.append(lastDate.time().toString());
        return lastMod;
    }

    if (index.column() == 1 && role == Qt::DisplayRole)
        if (fileInfo(index).isDir())
            return QString::number(QDir(filePath(index)).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).count()) + " Entrie(s)";

    if (index.column() == 0 && role == Qt::ForegroundRole)
    {
        if(fileInfo(index).isSymLink())
            return Qt::darkGray;
//        if(filePath(index.parent()) == QDir::homePath())
//            return Qt::white;
    }

    if ( role == Qt::FontRole )
        if ( ViewContainer *viewContainer = qobject_cast<ViewContainer *>(m_parent) )
            if ( QAbstractItemView *view = viewContainer->currentView())
            {
                QFont font = view->font();
                font.setBold(view->selectionModel()->selectedIndexes().contains(index));
                return font;
            }

    if ( index.column() == 4 )
    {
        if ( role == Qt::DisplayRole )
        {
            QString permission;

            const QFileInfo file = fileInfo(index);
            permission.append(file.permission(QFile::ReadUser) ? "r, " : "-, ");
            permission.append(file.permission(QFile::WriteUser) ? "w, " : "-, ");
            if(!file.isDir())
                permission.append(file.isExecutable() && !fileInfo(index).isDir() ? "x, " : "-, ");
            permission.append(file.owner());

            return permission;
        }
        return QVariant();
    }

    if ( role == Qt::TextAlignmentRole )
    {
        int align = Qt::AlignVCenter | Qt::AlignRight;
        if (index.column() == 1)
            return align;
        if (index.column() == 3)
            return QVariant(Qt::AlignVCenter | Qt::AlignLeft);
    }

    // sighs.... thumbnails... reflections... and other image/picture related shit.

    if (index.column() == 0 && ( role == Qt::DecorationRole || role == FlowPic || role == DecoImage ) && fileInfo(index).isDir())
    {
        const QSettings settings(QString("%1%2.directory").arg(filePath(index)).arg(QDir::separator()), QSettings::IniFormat);
        QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString());

        if (!icon.isNull())
        {
            if ( role == FlowPic )
                return icon.pixmap(SIZE);
            if ( role == DecoImage )
                return icon.pixmap(SIZE).toImage();
            return icon;
        }
    }

    if ( MainWindow::config.views.showThumbs )
        if ( ( role == Qt::DecorationRole || role == Thumbnail ) && index.column() == 0 )
        {
            const QImage &img = ThumbsLoader::thumb(filePath(index));
            if ( !img.isNull() )
            {
                if (role == Qt::DecorationRole)
                    return QIcon(QPixmap::fromImage(img));

                if (role == Thumbnail)
                    return img;
            }
        }
    if ( role == Reflection )
        return QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::Reflection));
    if ( role == FlowPic )
    {
        QPixmap p = QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::FlowPic));
        if ( !p.isNull() )
            return p;
        return qvariant_cast<QIcon>(QFileSystemModel::data(index, Qt::DecorationRole)).pixmap(SIZE);
    }
    if ( role == DecoImage )
        return qvariant_cast<QIcon>(QFileSystemModel::data(index, Qt::DecorationRole)).pixmap(SIZE).toImage();
    return QFileSystemModel::data(index, role);
}

QVariant
FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(section == 4 && role == Qt::DisplayRole)
        return "Permissions";
    if(section == 3 && role == Qt::DisplayRole)
        return "Last Modified";
    if ( role == Qt::SizeHintRole )
        return QSize(-1, QApplication::fontMetrics().height()+6);
    if ( role == Qt::DecorationRole )
        return QVariant();

    return QFileSystemModel::headerData(section, orientation, role);
}

QString
FileSystemModel::fullPath(const QModelIndex &index)
{
    if (!index.isValid())
        return QString();

    QStringList path;
    QModelIndex idx = index;

    while (idx.isValid())
    {
        path.prepend(idx.data().toString());
        idx = idx.parent();
    }

    QString fullPath = QDir::fromNativeSeparators(path.join(QDir::separator()));
#if !defined(Q_OS_WIN) || defined(Q_OS_WINCE)
    if ((fullPath.length() > 2) && fullPath[0] == QLatin1Char('/') && fullPath[1] == QLatin1Char('/'))
        fullPath = fullPath.mid(1);
#endif
    return fullPath;
}

QIcon
FileSystemModel::fullIcon(const QModelIndex &index)
{
    QFileIconProvider ip;
    return ip.icon(QFileInfo(fullPath(index)));
}

bool
FileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!data->hasUrls())
        return false;

    bool error = false;
    int ret = QMessageBox::question(APP->mainWindow(), tr("Are you sure?"), tr("you are about to move some stuff..."), QMessageBox::Yes, QMessageBox::No);
    if (ret == QMessageBox::Yes)
    {
        QStringList cp;
        foreach(QUrl url, data->urls())
            cp << url.toLocalFile();
        IO::Job::copy(cp, filePath(parent));
    }

    return error;
}

int
FileSystemModel::columnCount(const QModelIndex &parent) const
{
    return QFileSystemModel::columnCount(parent) + 1;
}

QStringList
FileSystemModel::supportedThumbs( const bool &filter )
{
    QStringList sf;
    foreach(QByteArray ar, QImageReader::supportedImageFormats())
        if ( filter )
            sf << QString( "*" + ar + "*" );
        else
            sf << ar;
    return sf;
}


