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

#include <QBitmap>
#include "dataloader.h"
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
    if (!QFileInfo(file).exists())
        emit noLongerExists(file);
    QMutexLocker locker(&m_mtx);
    if (m_data.contains(file))
        delete m_data.take(file);
}

Data
*DataLoader::_data(const QString &file)
{
    m_mtx.lock();
    const bool hasData = m_data.contains(file);
    m_mtx.unlock();
    if (hasData)
    {
        m_mtx.lock();
        Data *data = m_data.value(file);
        m_mtx.unlock();
        const bool isChecked = data->lastModified == QFileInfo(file).lastModified().toString();
        if (isChecked)
            return data;

        delete m_data.take(file);
    }
    if (enqueue(file) && isPaused())
        setPause(false);
    return 0;
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
    if (fi.isDir())
    {
        const QString &dirFile(fi.absoluteFilePath());
        const QDir dir(dirFile);
#if defined(Q_OS_UNIX)
        const QString &settingsFile(dir.absoluteFilePath(".directory"));
        const QSettings settings(settingsFile, QSettings::IniFormat);
        const QString &iconName = settings.value("Desktop Entry/Icon").toString();
        if (!iconName.isEmpty())
            data->iconName = iconName;
#endif
        QString count;
        int c = dir.entryList(allEntries).count();
        const QString &children = QString::number(c);
        if (c > 1)
            count = QString("%1%2").arg(children, " Entries");
        else if (c == 1)
            count = QString("%1%2").arg(children, " Entry");
        else
            count = QString("Empty");
        data->count = count;
        data->mimeType = m_mimeProvider.getMimeType(path);
        data->fileType = m_mimeProvider.getFileType(path);
        data->lastModified = fi.lastModified().toString();
        m_mtx.lock();
        m_data.insert(path, data);
        m_mtx.unlock();
        emit newData(path);
        return;
    }
    QImage image;
    if (!APP->activeThumbIfaces().isEmpty() && Store::config.views.showThumbs)
        for (int i = 0; i<APP->activeThumbIfaces().count(); ++i)
            if (APP->activeThumbIfaces().at(i)->thumb(path, m_extent, image))
            {
                data->thumb = image;
                break;
            }
    data->mimeType = m_mimeProvider.getMimeType(path);
    QString iconName = data->mimeType;
    iconName.replace("/", "-");
    data->iconName = iconName;
    data->lastModified = fi.lastModified().toString();
    data->fileType = m_mimeProvider.getFileType(path);
    m_mtx.lock();
    m_data.insert(path, data);
    m_mtx.unlock();
    emit newData(path);
}

bool
DataLoader::enqueue(const QString &file)
{
    QMutexLocker locker(&m_mtx);
    if (m_queue.contains(file))
        return false;
    m_queue.enqueue(file);
    return true;
}

void
DataLoader::_clearQueue()
{
    QMutexLocker locker(&m_mtx);
    m_queue.clear();
}

bool
DataLoader::hasQueue() const
{
    QMutexLocker locker(&m_mtx);
    return !m_queue.isEmpty();
}

QString
DataLoader::dequeue()
{
    QMutexLocker locker(&m_mtx);
    return m_queue.dequeue();
}

void
DataLoader::run()
{
    foreach (ThumbInterface *ti, APP->thumbIfaces())
        ti->init();

    while (!m_quit)
    {
        while (hasQueue())
            getData(dequeue());
        setPause(!m_quit);
        pause();
    }
}
