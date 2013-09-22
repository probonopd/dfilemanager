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
#if 0
    if ( Store::config.icons.customIcons.contains(info.filePath()) )
        return QIcon(Store::config.icons.customIcons.value(info.filePath()));
#endif
    if ( info.isDir() && s_themedDirs.contains(info.filePath()) )
        return s_themedDirs.value(info.filePath());
    QIcon icn = QFileIconProvider::icon(info);
    if ( QIcon::hasThemeIcon(icn.name()) )
        return QIcon::fromTheme(icn.name());
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

FileSystemModel::FileSystemModel(QObject *parent)
    : QFileSystemModel(parent)
    , m_container(static_cast<ViewContainer *>(parent))
    , m_history(new History(this))
{
    setResolveSymlinks(false);
    setIconProvider(m_iconProvider = new FileIconProvider(this));
    setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
    setNameFilterDisables(false);
    setReadOnly(false);
    m_nameThumbs = supportedThumbs();
    connect ( this, SIGNAL(rootPathChanged(QString)), this, SLOT(emitRootIndex(QString)) );
    m_thumbsLoader = new ThumbsLoader(this);
    connect ( m_thumbsLoader, SIGNAL(thumbFor(QString)), this, SLOT(thumbFor(QString)) );
    m_it = new ImagesThread(this);
    connect ( m_it, SIGNAL(imagesReady(QString)), this, SLOT(flowDataAvailable(QString)) );
    connect ( this, SIGNAL(rootPathChanged(QString)), m_history, SLOT(addPath(QString)) );
    connect ( this, SIGNAL(rootPathChanged(QString)), m_it, SLOT(clearData()) );
}

FileSystemModel::~FileSystemModel()
{
    m_thumbsLoader->clearQueue();
    m_it->clearQueue();
    while ( m_thumbsLoader->isRunning() )
        m_thumbsLoader->wait();
    while ( m_it->isRunning() )
        m_it->wait();
}

void
FileSystemModel::thumbFor(const QString &file)
{
    const QModelIndex &idx = index(file);
    emit dataChanged(idx, idx);
    m_it->queueFile(file, m_thumbsLoader->thumb(file));
}

void
FileSystemModel::flowDataAvailable(const QString &file)
{
    const QModelIndex &idx = index(file);
    emit flowDataChanged(idx, idx);
}

bool FileSystemModel::hasThumb(const QString &file) { return m_thumbsLoader->hasThumb(file); }

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

    if ( role == Qt::FontRole /*&& s_currentView*/ )
    {
        QFont font = m_container->font();
        font.setBold(m_container->selectionModel()->selectedIndexes().contains(index));
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

    const QFileInfo &fi = fileInfo(index);
    const QString &file = fi.filePath();

    const bool customIcon = Store::config.icons.customIcons.contains(file);

    if ( index.column() == 0 && role == Qt::DecorationRole && customIcon )
        return QIcon(Store::config.icons.customIcons.value(file));

    if ( role < FlowImg )
        if (index.column() > 0
                || !supportedThumbs().contains(fi.suffix(), Qt::CaseInsensitive)
                || !Store::config.views.showThumbs)
            return QFileSystemModel::data(index, role);

    if ( role == Qt::DecorationRole )
    {
        if ( m_thumbsLoader->hasThumb(file) )
            return QIcon(QPixmap::fromImage(m_thumbsLoader->thumb(file)));
        else
        {
            m_thumbsLoader->queueFile(file);
            return iconProvider()->icon(QFileInfo(file));
        }
    }

    if ( role >= FlowImg )
    {
        const QIcon &icon = customIcon ? QIcon(Store::config.icons.customIcons.value(file)) : iconProvider()->icon(fi);

        if ( m_it->hasData(file) )
            return QPixmap::fromImage(m_it->flowData(file, role == FlowRefl));
        if ( !m_thumbsLoader->hasThumb(file) )
            m_thumbsLoader->queueFile(file);

        if ( m_it->hasNameData(icon.name()) )
            return QPixmap::fromImage(m_it->flowNameData(icon.name(), role == FlowRefl));
        else
            m_it->queueName(icon);

        if ( role == FlowRefl )
            return QPixmap();

        return icon.pixmap(SIZE);
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

static QStringList s_st[2];

QStringList
FileSystemModel::supportedThumbs( const bool &filter )
{
    if ( s_st[filter].isEmpty() )
    foreach (QByteArray ar, QImageReader::supportedImageFormats())
        if ( filter )
            s_st[filter] << QString( "*" + ar + "*" );
        else
            s_st[filter] << ar;
    return s_st[filter];
}


