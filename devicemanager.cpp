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


#include "devicemanager.h"

using namespace DFM;

static inline bool isPartition(const QString &device)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, device,"org.freedesktop.DBus.Properties", "Get");
    msg << "org.freedesktop.UDisks.Device" << "DeviceIsPartition";
    return qvariant_cast<QDBusVariant>(QDBusConnection::systemBus().call(msg).arguments().at(0)).variant().toBool();
}

static inline bool isSwapDevice(const QString &device)
{
    return DbusCalls::deviceInfo(device, "PartitionType").toString() == "0x82";
}

DeviceManager *instance = 0;

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    QDBusConnection::systemBus().connect(SERVICE, PATH, INTERFACE_DISKS, "DeviceAdded", this, SLOT(deviceAdded(QDBusObjectPath)));
    QDBusConnection::systemBus().connect(SERVICE, PATH, INTERFACE_DISKS, "DeviceRemoved", this, SLOT(deviceRemoved(QDBusObjectPath)));
    QDBusConnection::systemBus().connect(SERVICE, PATH, INTERFACE_DISKS, "DeviceChanged", this, SLOT(deviceChanged(QDBusObjectPath)));
    m_tree = static_cast<QTreeWidget*>(parent);
    QTimer::singleShot(200, this, SLOT(populateLater()));
}

DeviceManager
*DeviceManager::manage(QTreeWidget *tw)
{
    if (!instance) instance = new DeviceManager(tw);
    return instance;
}

void
DeviceManager::deviceAdded(const QDBusObjectPath &device)
{
    if (isPartition(device.path()))
    {
        qDebug() << "device" << realDevPath(device.path()) << "added";
        const QString devPath(realDevPath(device.path()));
        m_items[devPath] = DeviceItem::initItem(m_devicesParent, devPath, m_tree);;
    }
}

void
DeviceManager::deviceRemoved(const QDBusObjectPath &device)
{
    qDebug() << "device" << realDevPath(device.path()) << "removed";
    delete m_items.take(realDevPath(device.path()));
}

void
DeviceManager::deviceChanged(const QDBusObjectPath &device)
{
    if ( DeviceItem *d = m_items[realDevPath(device.path())] )
        d->changeState();
}

void
DeviceManager::populateLater()
{
    m_devicesParent = new QTreeWidgetItem(m_tree);
    for (int i = 0; i<4; ++i)
        m_devicesParent->setText( i, "Devices" );
    foreach ( const QString &devPath, DbusCalls::devices() )
        if (!isSwapDevice(devPath)) //no swap devices.... you don't exactly mount those
        {
            DeviceItem *d = DeviceItem::initItem(m_devicesParent, devPath, m_tree);
            connect (d, SIGNAL(usageChanged(QTreeWidgetItem*)), this, SIGNAL(usageChanged(QTreeWidgetItem*)));
            m_items[devPath] = d;
        }
    m_devicesParent->setExpanded(true);
}

DeviceItem
*DeviceManager::deviceItemForFile(const QString &file)
{
    QString s(file.isEmpty() ? "/" : file);
    foreach ( DeviceItem *item, instance->m_items.values() )
        if ( s == item->mountPath() )
            return item;
    return deviceItemForFile(s.mid(0, s.lastIndexOf("/")));
}
