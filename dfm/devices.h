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


#ifndef DEVICES_H
#define DEVICES_H

#include <QMap>
#include "operations.h"

#if defined(HASSOLID)
#include "solid/device.h"
#include "solid/block.h"
#include "solid/storagedrive.h"
#include "solid/storagevolume.h"
#include "solid/storageaccess.h"
#include "solid/devicenotifier.h"
#endif

namespace DFM
{

class Device : public QObject
{
    Q_OBJECT
public:
#if defined(HASSOLID)
    Device(Solid::Device solid = Solid::Device());
#endif
    Device(const QFileInfo &dev = QFileInfo());
    ~Device();
    void setMounted(const bool mount);
    bool isMounted() const;
    QString mountPath() const;
    QString devPath() const;
    QString product() const;
    QString description() const;
    quint64 usedBytes() const;
    quint64 freeBytes() const;
    quint64 totalBytes() const;
    int used() const;
    inline void toggleMount() { setMounted(!isMounted()); }

signals:
    void accessibilityChanged(bool m, const QString &a);

private:
#if defined(HASSOLID)
    Solid::Device m_solid;
#endif
    QFileInfo m_fileInfo;
};

class Devices : public QObject
{
    Q_OBJECT
public:
    static Devices *instance();
    QList<Device *> devices() { return m_devices.values(); }
    QStringList mounts() const;
    void removeMount(const QString &mount);
    bool isDevice(const QString &path) { return m_mounts.contains(path); }

signals:
    void deviceAdded(Device *dev);
    void deviceRemoved(Device *dev);

private slots:
    void populate();
#if defined(HASSOLID)
    void deviceAdded(const QString &dev);
    void deviceRemoved(const QString &dev);
#endif

protected:
    Devices(QObject *parent = 0);

private:
    static Devices *m_instance;
    QMap<QString, Device *> m_devices;
    QTimer *m_timer;
    QStringList m_mounts;
    friend class Device;
};

}

#endif // DEVICES_H
