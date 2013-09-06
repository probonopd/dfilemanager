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

#include <QImageReader>

#include "filesystemmodel.h"
#include "iojob.h"
#include "thumbsloader.h"
#include "mainwindow.h"
#include "viewcontainer.h"

using namespace DFM;

static QAbstractItemView *s_currentView = 0;
static QMap<QString, QIcon> s_themedDirs;

FileIconProvider::FileIconProvider(FileSystemModel *model)
    : QFileIconProvider()
    , m_fsModel(model)
{
    connect ( model, SIGNAL(directoryLoaded(QString)), this, SLOT(loadThemedFolders(QString)) );
    connect ( this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), model, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
}

QIcon
FileIconProvider::icon(const QFileInfo &info) const
{
    if ( info.isDir() && s_themedDirs.contains(info.filePath()) )
        return s_themedDirs.value(info.filePath());
//    QIcon icn = QFileIconProvider::icon(info);
//    if ( icn.name() == "application-octet-stream" )
//        icn = QIcon::fromTheme("application-octet-stream");
    return QFileIconProvider::icon(info);
}

void
FileIconProvider::loadThemedFolders(const QString &path)
{
    foreach ( const QString &file, QDir(path).entryList(QDir::NoDotAndDotDot|QDir::AllDirs) )
    {
        const QString &filePath = QString("%1%2%3").arg(path).arg(QDir::separator()).arg(file);
        const QSettings settings(QString("%1%2.directory").arg(filePath).arg(QDir::separator()), QSettings::IniFormat);
        QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString());
        if ( !icon.isNull() && !s_themedDirs.contains(filePath) )
        {
            s_themedDirs.insert(filePath, icon);
            const QModelIndex &index = m_fsModel->index(filePath);
            emit dataChanged(index, index);
        }
    }
}

FileSystemModel::FileSystemModel(QObject *parent) :
    QFileSystemModel(parent)
{
    setResolveSymlinks(false);
    setIconProvider(m_iconProvider = new FileIconProvider(this));
    setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
    setNameFilterDisables(false);
    setReadOnly(false);
    connect ( MainWindow::currentWindow(), SIGNAL(viewChanged(QAbstractItemView*)), this, SLOT(setCurrentView(QAbstractItemView*)) );
    m_nameThumbs = supportedThumbs();
    connect ( this, SIGNAL(rootPathChanged(QString)), this, SLOT(emitRootIndex(QString)) );
}

void
FileSystemModel::setCurrentView(QAbstractItemView *view)
{
    s_currentView = view;
}

QPixmap
FileSystemModel::iconPix(const QFileInfo &info, const int &extent)
{
    const QSettings settings(QString("%1%2.directory").arg(info.filePath()).arg(QDir::separator()), QSettings::IniFormat);
    QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString());
    if ( icon.isNull() )
        icon = QIcon::fromTheme("inode-directory");
    if ( icon.isNull() )
        return QPixmap();

    int actSize = extent;
    while ( !icon.availableSizes().contains(QSize(actSize, actSize)) && actSize != 256 )
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
        if(fileInfo(index).isSymLink())
            return QColor(127, 127, 127, 255);

    if ( role == Qt::FontRole && s_currentView )
    {
        QFont font = s_currentView->font();
        font.setBold(s_currentView->selectionModel()->selectedIndexes().contains(index));
        return font;
    }

    if ( index.column() == 4 && role == Qt::DisplayRole )
    {
        QString permission;

        const QFileInfo file = fileInfo(index);
        permission.append(file.permission(QFile::ReadUser) ? "r, " : "-, ");
        permission.append(file.permission(QFile::WriteUser) ? "w, " : "-, ");
        if (!file.isDir())
            permission.append(file.isExecutable() && !fileInfo(index).isDir() ? "x, " : "-, ");
        permission.append(file.owner());

        return permission;
    }

    if ( role == Qt::TextAlignmentRole )
    {
        if (index.column() == 1)
            return int(Qt::AlignVCenter | Qt::AlignRight);
        if (index.column() == 3)
            return int(Qt::AlignVCenter | Qt::AlignLeft);
    }

    if ( index.column() == 0
         && role == Qt::DecorationRole
         && fileInfo(index).isDir() )
        return iconProvider()->icon(fileInfo(index));

    if ( role == IconName )
        return iconProvider()->icon(fileInfo(index)).name();

    //thumbnails in views...

    if ( Configuration::config.views.showThumbs
         && ( role == Qt::DecorationRole || role == Thumbnail )
         && index.column() == 0 )
    {
        const QPixmap &pix = QPixmap::fromImage(ThumbsLoader::thumb(filePath(index)));
        if ( !pix.isNull() )
        {
            if (role == Qt::DecorationRole)
                return QIcon(pix);

            if (role == Thumbnail)
                return pix.toImage();
        }
    }

    //in flowview

    if ( role == Reflection )
        return QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::Reflection));

    if ( role == FlowPic )
    {
        QPixmap p = iconProvider()->icon(fileInfo(index)).pixmap(SIZE);;
        if ( Configuration::config.views.showThumbs )
            if ( !QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::FlowPic)).isNull() )
                p = QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::FlowPic));
        if ( !p.isNull() )
            return p;
    }

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

bool
FileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!data->hasUrls())
        return false;

    IO::Job::copy(data->urls(), filePath(parent), true, true);
    return true;
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


