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
#include <QMutexLocker>

using namespace DFM;

DataLoader *DataLoader::m_instance = 0;

static QHash<QString, Data *> s_data;

DataLoader::DataLoader(QObject *parent) :
    Thread(parent),
    m_extent(256)
{
    connect(APP,SIGNAL(aboutToQuit()),this,SLOT(discontinue()));
    start();
}

DataLoader
*DataLoader::instance()
{
    if (!m_instance)
        m_instance = new DataLoader(APP);
    return m_instance;
}

void
DataLoader::fileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    const QString &file = QDir(path).absoluteFilePath(oldName);
    _removeData(file);
}

void
DataLoader::_removeData(const QString &file)
{
    QMutexLocker locker(&m_dataMutex);
    if (s_data.contains(file))
    {

        Data *data = s_data.take(file);
        m_dateCheck.remove(file);
        delete data;
    }
}

Data
*DataLoader::_data(const QString &file)
{
    if (s_data.contains(file))
        return s_data.value(file);
    m_queueMutex.lock();
    const bool inQueue = m_queue.contains(file);
    m_queueMutex.unlock();
    if (inQueue)
    {
        m_queueMutex.lock();
        int i = m_queue.indexOf(file);
        m_queue.move(i, qMax(0, i-10));
        m_queueMutex.unlock();
        return 0;
    }
    m_queueMutex.lock();
    m_queue << file;
    m_queueMutex.unlock();
    if (isPaused())
        setPause(false);
    return 0;
}

void
DataLoader::clearQ()
{
    QMutexLocker locker(&m_queueMutex);
    m_queue.clear();
}

void
DataLoader::getData(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.isReadable())
    {
        _removeData(path);
        return;
    }
    Data *data = new Data();
#if defined(Q_OS_UNIX)
    if (fi.isDir())
    {
        const QString &dirFile(fi.absoluteFilePath());
        const QDir dir(dirFile);
        const QString &settingsFile(dir.absoluteFilePath(".directory"));
        const QSettings settings(settingsFile, QSettings::IniFormat);
        const QString &iconName = settings.value("Desktop Entry/Icon").toString();
        if (!iconName.isEmpty())
            data->iconName = iconName;

        QString count;
        int c = dir.entryList(allEntries).count();
        const QString &children = QString::number(c);
        if (c > 1)
            count = QString("%1%2").arg(children, " Entries");
        else
            count = QString("%1%2").arg(children, " Entry");
        data->count = count;
        data->mimeType = mimeProvider.getMimeType(path);
        m_dataMutex.lock();
        s_data.insert(path, data);
        m_dataMutex.unlock();
        emit newData(path, iconName);
        return;
    }
#endif
    QImage image;
    if (!APP->activeThumbIfaces().isEmpty() && Store::config.views.showThumbs)
        for (int i = 0; i<APP->activeThumbIfaces().count(); ++i)
            if (APP->activeThumbIfaces().at(i)->thumb(path, m_extent, image))
            {
                data->thumb = image;
                break;
            }
    data->mimeType = mimeProvider.getMimeType(path);
    QString iconName = data->mimeType;
    iconName.replace("/", "-");
    data->iconName = iconName;
    m_dataMutex.lock();
    m_dateCheck.insert(path, fi.lastModified().toString());
    s_data.insert(path, data);
    m_dataMutex.unlock();
    emit newData(path, iconName);
}

void
DataLoader::run()
{
    foreach (ThumbInterface *ti, APP->thumbIfaces())
        ti->init();

    while (!m_quit)
    {
        while (!m_queue.isEmpty())
        {
            m_queueMutex.lock();
            const QString &file = m_queue.takeFirst();
            m_queueMutex.unlock();
            getData(file);
        }
        setPause(!m_quit);
        pause();
    }
}

///////////////////////////////////////////////////////////////////////

static QHash<QString, QImage> s_themeIcons[2];

ImagesThread::ImagesThread(QObject *parent)
    : Thread(parent)
{}

void
ImagesThread::removeData(const QString &file)
{
    QMutexLocker locker(&m_thumbsMutex);
    for (int i = 0; i < 2; ++i)
        m_images[i].remove(file);
}

void
ImagesThread::genImagesFor(const QPair<QString, QImage> &strImg)
{
    if (!strImg.first.isNull() || strImg.second.isNull())
    {
        m_thumbsMutex.lock();
        m_images[0].insert(strImg.first, Ops::flowImg(strImg.second));
        m_images[1].insert(strImg.first, Ops::reflection(strImg.second));
        m_thumbsMutex.unlock();
        emit imagesReady(strImg.first);
    }
}

void
ImagesThread::genNameIconsFor(const QPair<QString, QImage> &strImg)
{
    if (!strImg.first.isNull() || strImg.second.isNull())
    {
        m_thumbsMutex.lock();
        s_themeIcons[0].insert(strImg.first, Ops::flowImg(strImg.second));
        s_themeIcons[1].insert(strImg.first, Ops::reflection(strImg.second));
        m_thumbsMutex.unlock();
    }
}

void ImagesThread::run()
{
    while (!m_quit)
    {
        while (!m_nameQueue.isEmpty())
        {
            m_queueMutex.lock();
            const QPair<QString, QImage> &vals = m_nameQueue.take(m_nameQueue.keys().first());
            m_queueMutex.unlock();
            genNameIconsFor(vals);
        }
        while (!m_imgQueue.isEmpty())
        {
            m_queueMutex.lock();
            const QPair<QString, QImage> &vals = m_imgQueue.take(m_imgQueue.keys().first());
            m_queueMutex.unlock();
            genImagesFor(vals);
        }
        setPause(!m_quit);
        pause();
    }
}

QImage
ImagesThread::flowData(const QString &file, const bool refl)
{
    QMutexLocker locker(&m_thumbsMutex);
    if (m_images[refl].contains(file))
        return m_images[refl].value(file);
    return QImage();
}

QImage
ImagesThread::flowNameData(const QString &name, const bool refl)
{
    QMutexLocker locker(&m_thumbsMutex);
    if (s_themeIcons[refl].contains(name))
        return s_themeIcons[refl].value(name);
    return QImage();
}

bool
ImagesThread::hasData(const QString &file)
{
    QMutexLocker locker(&m_thumbsMutex);
    return m_images[0].contains(file);
}

bool
ImagesThread::hasNameData(const QString &name)
{
    QMutexLocker locker(&m_thumbsMutex);
    return s_themeIcons[0].contains(name);
}

void
ImagesThread::queueName(const QIcon &icon)
{
    const QString &name = icon.name();
    if (hasNameData(name))
        return;

    m_queueMutex.lock();
    const bool inQueue = m_nameQueue.contains(name);
    m_queueMutex.unlock();
    if (inQueue)
        return;

    m_queueMutex.lock();
    m_nameQueue.insert(name, QPair<QString, QImage>(icon.name(), icon.pixmap(SIZE).toImage()));
    m_queueMutex.unlock();
    if (isPaused())
        setPause(false);
    if (!isRunning())
        start();
}

void
ImagesThread::queueFile(const QString &file, const QImage &source, const bool force)
{
    if (hasData(file) && !force)
        return;

    m_queueMutex.lock();
    const bool inQueue = m_imgQueue.contains(file);
    m_queueMutex.unlock();
    if (inQueue && !force)
        return;

    m_queueMutex.lock();
    m_imgQueue.insert(file, QPair<QString, QImage>(file, source));
    m_queueMutex.unlock();

    if (isPaused())
        setPause(false);
    if (!isRunning())
        start();
}
