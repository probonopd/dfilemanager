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
#include <QMessageBox>

#include "filesystemmodel.h"
#include "iojob.h"
#include "thumbsloader.h"
#include "mainwindow.h"
#include "viewcontainer.h"

using namespace DFM;

static QAbstractItemView *s_currentView = 0;

FileSystemModel::FileSystemModel(QObject *parent) :
    QFileSystemModel(parent)
{
    setResolveSymlinks(false);
    setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
    setNameFilterDisables(false);
    setReadOnly(false);
    connect ( MainWindow::currentWindow(), SIGNAL(viewChanged(QAbstractItemView*)), this, SLOT(setCurrentView(QAbstractItemView*)) );
    m_nameThumbs = supportedThumbs();
    connect ( this, SIGNAL(rootPathChanged(QString)), this, SLOT(emitRootIndex(QString)) );
    connect ( this, SIGNAL(directoryLoaded(QString)), this, SLOT(dirLoaded(QString)) );
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

void
FileSystemModel::dirLoaded(const QString &path)
{
    for ( int i = 0; i < rowCount(index(path)); ++i )
    {
        if ( !QFileInfo(path).isDir() )
            continue;
        const QModelIndex &idx = index(i, 0, index(path));
        const QSettings settings(QString("%1%2.directory").arg(filePath(idx)).arg(QDir::separator()), QSettings::IniFormat);
        QIcon icon = QIcon::fromTheme(settings.value("Desktop Entry/Icon").toString());
        if ( !icon.isNull() )
        {
            m_themedDirs.insert(filePath(idx), icon);
            emit dataChanged(idx, idx);
        }
    }
}

QVariant
FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this)
        return QVariant();

    if ( role == IconName )
    {
        const QIcon &icon = qvariant_cast<QIcon>(QFileSystemModel::data(index, role));
        if ( !icon.isNull() )
            return icon.name();
    }

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
            return QColor(127, 127, 127, 255);
//        if(filePath(index.parent()) == QDir::homePath())
//            return Qt::white;
    }

    if ( role == Qt::FontRole && s_currentView )
    {
        QFont font = s_currentView->font();
        font.setBold(s_currentView->selectionModel()->selectedIndexes().contains(index));
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

    if (index.column() == 0
            && ( role == Qt::DecorationRole || role == FlowPic )
            && fileInfo(index).isDir()
            && m_themedDirs.contains(filePath(index)))
    {
        const QIcon &icon = m_themedDirs.value(filePath(index));

        if (!icon.isNull())
        {
            if ( role == FlowPic )
                return icon.pixmap(SIZE);
            return icon;
        }
    }

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

    if ( role == Reflection )
        return QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::Reflection));

    if ( role == FlowPic )
    {
        QPixmap p;
        if ( Configuration::config.views.showThumbs )
            p = QPixmap::fromImage(ThumbsLoader::thumb(filePath(index), ThumbsLoader::FlowPic));
        else
            p = iconProvider()->icon(fileInfo(index)).pixmap(SIZE);
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

    bool error = false;
    int ret = QMessageBox::question(MainWindow::currentWindow(), tr("Are you sure?"), tr("you are about to move some stuff..."), QMessageBox::Yes, QMessageBox::No);
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


