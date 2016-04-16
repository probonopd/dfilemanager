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
#include "operations.h"
#include "config.h"
#include "application.h"
#include "interfaces.h"
#include <QMutexLocker>
#include <QImageReader>
#include <QWaitCondition>
#include <QDateTime>
#include <QDebug>

using namespace DFM;

DDataLoader *DDataLoader::s_instance = 0;
DHash<QString, Data *> DDataLoader::s_data;
DQueue<QString> DDataLoader::s_queue;
DMimeProvider DDataLoader::s_mimeProvider;

DDataLoader::DDataLoader(QObject *parent) :
    QThread(parent),
    m_extent(256)
{
    connect(this, SIGNAL(started()), this, SLOT(init()));
    connect(this, SIGNAL(dataRequested()), this, SLOT(loadData()), Qt::QueuedConnection);
    moveToThread(this);
    start();
}

void
DDataLoader::init()
{
    dApp->loadPlugins();
    foreach (ThumbInterface *ti, dApp->thumbIfaces())
        ti->init();
}

DDataLoader
*DDataLoader::instance()
{
    if (!s_instance)
        s_instance = new DDataLoader();
    return s_instance;
}

void
DDataLoader::fileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    const QString &file = QDir(path).absoluteFilePath(oldName);
    removeData(file);
}

void
DDataLoader::removeData(const QString &file)
{
    if (!QFileInfo(file).exists())
        emit s_instance->noLongerExists(file);
    s_data.destroy(file);
}

Data
*DDataLoader::data(const QString &file, const bool checkOnly)
{
    if (Data *data = s_data.value(file, 0))
    {
        const bool isChecked = data->lastModified == QFileInfo(file).lastModified().toString();
        if (isChecked)
            return data;

        s_data.destroy(file);
    }
    if (checkOnly)
        return 0;
    if (s_queue.enqueue(file))
        emit instance()->dataRequested();
    return 0;
}

void
DDataLoader::getData(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.isReadable() || !fi.exists())
    {
        removeData(path);
        return;
    }
    Data *data = new Data();
    if (fi.isDir())
    {
        const QString &dirFile(fi.absoluteFilePath());
        const QDir dir(dirFile);
#if defined(ISUNIX)
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
        data->mimeType = s_mimeProvider.getMimeType(path);
        data->fileType = s_mimeProvider.getFileType(path);
        data->lastModified = fi.lastModified().toString();
        s_data.insert(path, data);
        emit newData(path);
        return;
    }
    QImage image;
    const QString mime(s_mimeProvider.getMimeType(path));

    if (!dApp->activeThumbIfaces().isEmpty() && Store::config.views.showThumbs)
    for (int i = 0; i<dApp->activeThumbIfaces().count(); ++i)
    if (dApp->activeThumbIfaces().at(i)->thumb(path, mime, image, m_extent))
    {
        data->thumb = image;
        break;
    }

    data->mimeType = mime;
    QString iconName = mime;
    iconName.replace("/", "-");
    data->iconName = iconName;
    data->lastModified = fi.lastModified().toString();
    data->fileType = s_mimeProvider.getFileType(path);
    s_data.insert(path, data);
    emit newData(path);
}

void
DDataLoader::loadData()
{
    while (!s_queue.isEmpty())
        getData(s_queue.dequeue());
}
