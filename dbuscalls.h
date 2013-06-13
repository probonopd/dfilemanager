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


#ifndef DBUSCALLS_H
#define DBUSCALLS_H

#include <QFileInfo>
#include <QDir>

#if 0  //might need later
#include "mntent.h"
#include "unistd.h"
#include <sys/types.h>
#include <ustat.h>
#endif
#include <QtDBus/QtDBus>

namespace DFM
{
namespace DbusCalls
{

inline static QStringList devices()
{
    QStringList devFile;
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusMessage m = QDBusMessage::createMethodCall("org.freedesktop.UDisks", "/org/freedesktop/UDisks","org.freedesktop.UDisks", "EnumerateDevices");
    QDBusReply<QList<QDBusObjectPath> > reply = bus.call(m);
    if(reply.isValid())
        foreach(QDBusObjectPath path,reply.value())
        {
            QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.UDisks", path.path(),"org.freedesktop.DBus.Properties", "Get");
            msg << "org.freedesktop.UDisks.Device" << "DeviceIsPartition";
            QDBusMessage rply = bus.call(msg);
            if(QVariant(qvariant_cast<QDBusVariant>(rply.arguments().at(0)).variant()).toBool())  //if isPartition
            {
                QDBusMessage m = QDBusMessage::createMethodCall("org.freedesktop.UDisks", path.path(),"org.freedesktop.DBus.Properties", "Get");
                m << "org.freedesktop.UDisks.Device" << "DeviceFile";
                devFile << qvariant_cast<QDBusVariant>(bus.call(m).arguments().at(0)).variant().toString();
            }
        }
    return devFile;
}

inline static QVariant deviceInfo(QString dev, QString arg)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    const QString devName = QFileInfo(dev).fileName();
    QDBusMessage me = QDBusMessage::createMethodCall("org.freedesktop.UDisks", QString( "/org/freedesktop/UDisks/devices/%1" ).arg( devName ),"org.freedesktop.DBus.Properties", "Get");
    me << "org.freedesktop.UDisks.Device" << arg;
    return qvariant_cast<QDBusVariant>( bus.call( me ).arguments().at(0) ).variant();
}
}
}

#endif // DBUSCALLS_H
