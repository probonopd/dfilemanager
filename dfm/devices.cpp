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

#include <QDir>
#include <QTimer>
#include "devices.h"

using namespace DFM;

#if defined(HASSOLID)
Device::Device(Solid::Device solid):m_solid(solid)
{
    if (!m_solid.is<Solid::StorageAccess>())
    {
        deleteLater();
        return;
    }
    connect(m_solid.as<Solid::StorageAccess>(), SIGNAL(accessibilityChanged(bool, const QString &)), this, SIGNAL(accessibilityChanged(bool, const QString &)));
}
#endif

Device::Device(const QFileInfo &dev):m_fileInfo(dev){}

Device::~Device()
{
//#if defined(HASSOLID)
//#endif
}

bool
Device::isMounted() const
{
#if defined(HASSOLID)
    if (m_solid.isValid())
        if (const Solid::StorageAccess *sa = m_solid.as<Solid::StorageAccess>())
            return sa->isAccessible();
    return false;
#else
    return true;
#endif
}

void
Device::setMounted(const bool mount)
{
#if defined(HASSOLID)
    if (!m_solid.isValid())
        return;
    Solid::StorageAccess *sa(0);
    if (!(sa = m_solid.as<Solid::StorageAccess>()))
        return;

    if (mount)
        sa->setup();
    else
        sa->teardown();
#endif
}

QString
Device::mountPath() const
{
#if defined(HASSOLID)
    if (m_solid.isValid())
        if (const Solid::StorageAccess *sa = m_solid.as<Solid::StorageAccess>())
            return sa->filePath();
    return QString();
#else
    return m_fileInfo.filePath();
#endif
}

QString
Device::devPath() const
{
#if defined(HASSOLID)
    if (m_solid.isValid())
        if (const Solid::Block *block = m_solid.as<Solid::Block>())
            return block->device();
    return QString();
#else
    return m_fileInfo.filePath();
#endif
}

QString
Device::product() const
{
#if defined(HASSOLID)
    return m_solid.isValid() ? m_solid.product() : QString();
#else
    return QString();
#endif
}

quint64
Device::usedBytes() const
{
#if defined(HASSOLID)
    return Ops::getDriveInfo<Ops::Used>(mountPath());
#else
    return 0;
#endif
}

quint64
Device::freeBytes() const
{
#if defined(HASSOLID)
    return Ops::getDriveInfo<Ops::Free>(mountPath());
#else
    return 0;
#endif
}

quint64
Device::totalBytes() const
{
#if defined(HASSOLID)
    return Ops::getDriveInfo<Ops::Total>(mountPath());
#else
    return 0;
#endif
}

int
Device::used() const
{
#if defined(HASSOLID)
    return usedBytes() ? (int)(((float)usedBytes()/(float)totalBytes())*100) : 0;
#else
    return 0;
#endif
}

QString
Device::description() const
{
#if defined(HASSOLID)
    return m_solid.description();
#else
    return QString();
#endif
}

//-------------------------------------------------------------------------

Devices *Devices::m_instance = 0;

Devices
*Devices::instance()
{
    if (!m_instance)
        m_instance = new Devices();
    return m_instance;
}

Devices::Devices(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
#if defined(HASSOLID)
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), this, SLOT(deviceAdded(QString)));
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), this, SLOT(deviceRemoved(QString)));
#else // Q_OS_WIN
    connect (m_timer, SIGNAL(timeout()), this, SLOT(populate()));
    m_timer->start(5000);
#endif
    populate();
}

QStringList
Devices::mounts() const
{
    return m_mounts;
}

void
Devices::removeMount(const QString &mount)
{
    if (m_mounts.contains(mount))
        m_mounts.removeOne(mount);
}

#if defined(HASSOLID)
void
Devices::deviceAdded(const QString &dev)
{
    Solid::Device device = Solid::Device(dev);
    if (device.is<Solid::StorageAccess>())
    {
        Device *d = new Device(device);
        m_devices.insert(dev, d);
        emit deviceAdded(d);
        if (d->isMounted() && !m_mounts.contains(d->mountPath()))
            m_mounts << d->mountPath();
    }
}

void
Devices::deviceRemoved(const QString &dev)
{
    if (m_devices.contains(dev))
    {
        Device *d = m_devices.take(dev);
        emit deviceRemoved(d);
        if (d && d->isMounted() && m_mounts.contains(d->mountPath()))
        {
            m_mounts.removeOne(d->mountPath());
            delete d;
        }
    }
}
#endif

void
Devices::populate()
{
#if defined(HASSOLID)
    foreach (Solid::Device dev, Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess))
    {
        Device *d = new Device(dev);
        m_devices.insert(dev.udi(), d);
        if (d->isMounted() && !m_mounts.contains(d->mountPath()))
            m_mounts << d->mountPath();
    }
#else
    QFileInfoList drives = QDir::drives();
    for (int i = 0; i<drives.count(); ++i)
        if (!m_devices.contains(drives.at(i).filePath()))
        {
            Device *d = new Device(drives.at(i));
            m_devices.insert(drives.at(i).filePath(), d);
            emit deviceAdded(d);
        }
    foreach (const QString &dev, m_devices.keys())
        if (!drives.contains(QFileInfo(dev)))
        {
            Device *d = m_devices.take(dev);
            emit deviceRemoved(d);
        }
#endif
}
