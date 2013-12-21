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
#include <QMutexLocker>

using namespace DFM;

ThumbsLoader *ThumbsLoader::m_instance = 0;

ThumbsLoader::ThumbsLoader(QObject *parent) :
    QThread(parent),
    m_extent(256),
    m_quit(false)
{
    connect(APP,SIGNAL(aboutToQuit()),this,SLOT(discontinue()));
    start();
}

ThumbsLoader
*ThumbsLoader::instance()
{
    if (!m_instance)
        m_instance = new ThumbsLoader(APP);
    return m_instance;
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
    if ( m_thumbs.contains(file) )
    {
        m_thumbs.remove(file);
        m_dateCheck.remove(file);
    }
}

bool
ThumbsLoader::hasIcon(const QString &dir) const
{
    return m_icons.contains(dir);
}

QString
ThumbsLoader::icon(const QString &dir) const
{
    if ( hasIcon(dir) )
        return m_icons.value(dir);
    return QString();
}

bool
ThumbsLoader::hasThumb(const QString &file) const
{
    return m_thumbs.contains(file) && m_dateCheck.value(file) == QFileInfo(file).lastModified().toString();
}

void
ThumbsLoader::clearQ()
{
    QMutexLocker locker(&m_listMutex);
    m_queue.clear();
}

void
ThumbsLoader::queueFile(const QString &file)
{
    QMutexLocker locker(&m_listMutex);
    if ( m_queue.contains(file) || hasThumb(file) || hasIcon(file) /*|| m_tried.contains(file)*/ )
        return;

    m_queue << file;
    if ( m_pause )
        setPause(false);
}

QImage
ThumbsLoader::thumb(const QString &file) const
{
    if ( hasThumb(file) )
        return m_thumbs.value(file);
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
            m_icons.insert(dirFile, iconName);
            emit thumbFor(path, iconName);
        }
        return;
    }
    QImage image;
    if ( !APP->activeThumbIfaces().isEmpty() )
        for ( int i = 0; i<APP->activeThumbIfaces().count(); ++i )
            if (APP->activeThumbIfaces().at(i)->thumb(path, m_extent, image) )
            {
                m_dateCheck.insert(path, fi.lastModified().toString());
                m_thumbs.insert(path, image);
                emit thumbFor(path, QString());
                return;
            }
}

void
ThumbsLoader::run()
{
    foreach (ThumbInterface *ti, APP->thumbIfaces())
        ti->init();

    while (!m_quit)
    {
        while ( !m_queue.isEmpty() )
        {
            m_listMutex.lock();
            const QString &file = m_queue.takeFirst();
            m_listMutex.unlock();
            genThumb(file);
        }
        setPause(!m_quit);
        pause();
    }
}

///////////////////////////////////////////////////////////////////////

static QHash<QString, QImage> s_themeIcons[2];

ImagesThread::ImagesThread(QObject *parent)
    : QThread(parent)
    , m_quit(false)
{}

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
    const QImage &source = m_imgQueue.take(file);
    if ( !source.isNull() )
    {
        m_images[0].insert(file, Ops::flowImg(source));
        m_images[1].insert(file, Ops::reflection(source));
        emit imagesReady(file);
    }
}

void
ImagesThread::genNameIconsFor(const QString &name)
{
    const QImage &source = m_nameQueue.take(name);
    if ( !source.isNull() )
    {
        s_themeIcons[0].insert(name, Ops::flowImg(source));
        s_themeIcons[1].insert(name, Ops::reflection(source));
    }
}

void ImagesThread::run()
{
    while ( !m_quit )
    {
        while ( !m_nameQueue.isEmpty() )
            genNameIconsFor(m_nameQueue.keys().first());
        while ( !m_imgQueue.isEmpty() )
            genImagesFor(m_imgQueue.keys().first());

        setPause(!m_quit);
        pause();
    }
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
    if ( m_pause )
        setPause(false);
    if ( !isRunning() )
        start();
}

void
ImagesThread::queueFile(const QString &file, const QImage &source, const bool force )
{
    if ( (m_imgQueue.contains(file) || m_images[0].contains(file)) && !force )
        return;

    m_imgQueue.insert(file, source);
    if ( m_pause )
        setPause(false);
    if ( !isRunning() )
        start();
}
