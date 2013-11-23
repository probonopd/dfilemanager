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


#include "thumbsloader.h"
#include <QImage>
#include "columnsview.h"
#include "mainwindow.h"
#include "operations.h"
#include "config.h"
#include "viewcontainer.h"
#include "application.h"
#include <QBitmap>
#include <QAbstractItemModel>

using namespace DFM;

static QHash<QString, QImage> s_thumbs;
static QHash<QString, QString> s_dateCheck;
static QHash<QString, QString> s_icons;

ThumbsLoader::ThumbsLoader(QObject *parent) :
    QThread(parent),
    m_extent(256),
    m_fsModel(static_cast<DFM::FileSystemModel *>(parent))
{
    connect(m_fsModel, SIGNAL(fileRenamed(QString,QString,QString)), this, SLOT(fileRenamed(QString,QString,QString)));
    connect(m_fsModel, SIGNAL(modelAboutToBeReset()), this, SLOT(clearQueue()));
    connect(m_fsModel, SIGNAL(layoutAboutToBeChanged()), this, SLOT(clearQueue()));
}

void
ThumbsLoader::fileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    const QString &file = QDir(path).absoluteFilePath(oldName);
    removeThumb(file);
}

void
ThumbsLoader::removeThumb(const QString &file)
{
    if ( s_thumbs.contains(file) )
    {
        s_thumbs.remove(file);
        s_dateCheck.remove(file);
    }
}

bool
ThumbsLoader::hasIcon(const QString &dir) const
{
    return s_icons.contains(dir);
}

QString
ThumbsLoader::icon(const QString &dir) const
{
    if ( hasIcon(dir) )
        return s_icons.value(dir);
    return QString();
}

bool
ThumbsLoader::hasThumb(const QString &file) const
{
    return s_thumbs.contains(file) && s_dateCheck.value(file) == QFileInfo(file).lastModified().toString();
}

void
ThumbsLoader::queueFile(const QString &file)
{
    if ( m_queue.contains(file) || hasThumb(file) || hasIcon(file) || m_tried.contains(file) )
        return;

    m_queue << file;
    start();
}

QImage
ThumbsLoader::thumb(const QString &file) const
{
    if ( hasThumb(file) )
        return s_thumbs.value(file);
    return QImage();
}

void
ThumbsLoader::genThumb( const QString &path )
{
    const QFileInfo &fi(path);
    if ( !fi.isReadable() )
    {
        removeThumb(path);
        return;
    }

    if ( fi.isDir() )
    {
        const QString &dirFile(fi.absoluteFilePath());
        const QString &settingsFile(QDir(dirFile).absoluteFilePath(".directory"));
        const QSettings settings(settingsFile, QSettings::IniFormat);
        const QString &iconName = settings.value("Desktop Entry/Icon").toString();
        if ( !iconName.isEmpty() )
            if ( !hasIcon(dirFile) )
        {
            s_icons.insert(dirFile, iconName);
            const QModelIndex &index = m_fsModel->index(dirFile);
            emit thumbFor(path, iconName);
        }
        return;
    }

    QImage image;
    if ( !APP->plugins().isEmpty() )
        for ( int i = 0; i<APP->plugins().count(); ++i )
            if( ThumbInterface *ti = qobject_cast<ThumbInterface *>(APP->plugins().at(i)) )
                if ( ti->canRead(path) )
                    image = ti->thumb(path, m_extent);

    if ( !image.isNull() )
    {
        s_dateCheck.insert(path, fi.lastModified().toString());
        s_thumbs.insert(path, image);
        emit thumbFor(path, QString());
        return;
    }
    m_tried << path;
}

void ThumbsLoader::clearQueue() { m_queue.clear(); }

void
ThumbsLoader::run()
{
    while ( !m_queue.isEmpty() )
        genThumb(m_queue.takeFirst());
}


static QHash<QString, QImage> s_themeIcons[2];

ImagesThread::ImagesThread(QObject *parent)
    : QThread(parent)
    , m_fsModel(static_cast<FileSystemModel *>(parent))
{}

void ImagesThread::clearQueue() { m_imgQueue.clear(); }

void
ImagesThread::removeData(const QString &file)
{
    for ( int i = 0; i < 2; ++i )
        m_images[i].remove(file);
    m_imgQueue.remove(file);
}

void
ImagesThread::genImagesFor(const QString &file)
{
    if ( !QFileInfo(file).isReadable() )
        return;
    const QImage &source = m_imgQueue.value(file);
    if ( source.isNull() )
        return;
    m_images[0].insert(file, Ops::flowImg(source));
    m_images[1].insert(file, Ops::reflection(source));
    emit imagesReady(file);
    m_imgQueue.remove(file);
}

void
ImagesThread::genNameIconsFor(const QString &name)
{
    const QImage &source = m_nameQueue.value(name);
    if ( source.isNull() )
        return;

    s_themeIcons[0].insert(name, Ops::flowImg(source));
    s_themeIcons[1].insert(name, Ops::reflection(source));
    m_nameQueue.remove(name);
}

void ImagesThread::run()
{
    while ( !m_nameQueue.isEmpty() )
        genNameIconsFor(m_nameQueue.keys().first());
    while ( !m_imgQueue.isEmpty() )
        genImagesFor(m_imgQueue.keys().first());
}

QImage
ImagesThread::flowData(const QString &file, const bool refl)
{
    if ( hasData(file) )
        return m_images[refl].value(file);
    return QImage();
}

QImage
ImagesThread::flowNameData(const QString &name, const bool refl)
{
    if ( hasNameData(name) )
        return s_themeIcons[refl].value(name);
    return QImage();
}

bool
ImagesThread::hasData(const QString &file)
{
    return m_images[0].contains(file);
}

bool
ImagesThread::hasNameData(const QString &name)
{
    return s_themeIcons[0].contains(name);
}

void
ImagesThread::queueName(const QIcon &icon)
{
    if ( m_nameQueue.contains(icon.name()) || hasNameData(icon.name()) )
        return;

    const QImage &source = icon.pixmap(SIZE).toImage();
    m_nameQueue.insert(icon.name(), source);
    start();
}

void
ImagesThread::queueFile(const QString &file, const QImage &source, const bool force )
{
    if ( (m_imgQueue.contains(file) || m_images[0].contains(file)) && !force )
        return;

    m_imgQueue.insert(file, source);
    start();
}
