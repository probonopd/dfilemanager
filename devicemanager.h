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


#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>
#include <QMainWindow>
#include <QToolButton>
#include <QMessageBox>
#include "application.h"
#include "operations.h"
#include "iconprovider.h"

namespace DFM
{

#define SERVICE                 "org.freedesktop.UDisks"
#define PATH                    "/org/freedesktop/UDisks"
#define INTERFACE_DISKS         "org.freedesktop.UDisks"
#define INTERFACE_DISKS_DEVICE  "org.freedesktop.UDisks.Device"
#define UDI_DISKS_PREFIX        "/org/freedesktop/UDisks"

static inline QDBusMessage methodCall(const QString &devName, const QString &method)
{
    return QDBusMessage::createMethodCall( SERVICE, QString( "/org/freedesktop/UDisks/devices/%1" ).arg( devName ), INTERFACE_DISKS_DEVICE, method );
}

static inline QString realDevPath( const QString &path )
{
    return QString("/dev" + path.mid(path.lastIndexOf("/")));
}

static inline float realSize(const float size, int *t)
{
    float s = size;
    while (s > 1024.0)
    {
        if (*t == 5)
            break;
        s = s/1024.0;
        ++(*t);
    }
    return s;
}

static QString spaceType[6] = { " Byte", " KiB", " MiB", " GiB", " TiB", " PiB" };

static inline QPixmap mountIcon( bool mounted, int size, QColor col )
{
    QPixmap pix( size, size );
    pix.fill( Qt::transparent );
    QPainter p( &pix );
    QRect rect( pix.rect() );
    const int d = rect.height()/3;
    rect.adjust( d, d, -d, -d );
    p.setRenderHint( QPainter::Antialiasing );
    p.setPen( QPen( col, 2 ) );
    if ( mounted )
        p.setBrush( col );
    p.drawEllipse( rect );
    p.end();
    return pix;
}

class DeviceItem : public QObject, public QTreeWidgetItem //have to inherit QObject for signals/slots/eventHandling
{
    Q_OBJECT
public:
    explicit DeviceItem( QTreeWidgetItem *parentItem = 0, QStringList texts = QStringList(), QTreeWidget *view = 0, bool isMounted = false )
        : QTreeWidgetItem( parentItem, texts )
        , m_mounted(isMounted)
        , m_view(view)
        , m_mainWin(APP->mainWindow())
        , m_devPath(texts[3])
        , m_parentItem(parentItem)
        , m_tb(new QToolButton(m_view))
        , m_usedBytes(0)
        , m_freeBytes(0)
        , m_totalBytes(0)
        , m_timer(new QTimer(this))
    {
        m_tb->setIcon(mountIcon(m_mounted, 16, m_view->palette().color(m_view->foregroundRole())));
        m_tb->setVisible(true);
        m_tb->setFixedSize(16, 16);
        m_tb->setToolTip( m_mounted ? "Unmount" : "Mount" );
        connect (m_tb, SIGNAL(clicked()), this, SLOT(toggleMount()));
        connect (m_timer, SIGNAL(timeout()), this, SLOT(updateSpace()));
        m_timer->start(1000);
        updateSpace();
        if ( m_mounted )
            m_mountPath = DbusCalls::deviceInfo( m_devPath, "DeviceMountPaths" ).toString();
        else
        {
            int t = 0;
            QString size = QString::number( realSize( DbusCalls::deviceInfo(m_devPath, "PartitionSize").toFloat(), &t ) );
            int i = size.mid( size.indexOf(".")+3, 1).toInt();
            if ( i > 4 ) //if we have a higher value then 4 we round up....
                size.replace(size.indexOf(".")+2, 1, QString::number(size.mid( size.indexOf(".")+2, 1).toInt()+1) );
            size.chop( size.length() - (size.indexOf(".")+3) );
            setText(0, size + spaceType[t] + " Device");
        }

        //try and catch everything so the mount button is always on the right place...
        connect (m_view, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(viewEvent()));
        connect (m_view, SIGNAL(viewportEntered()), this, SLOT(viewEvent()));
    }
    ~DeviceItem() { m_tb->deleteLater(); }
    static DeviceItem *initItem( QTreeWidgetItem *parentItem = 0, const QString &devPath = "", QTreeWidget *view = 0 ) //creates a new item and returns a pointer
    {
        QString mountPath( DbusCalls::deviceInfo( devPath, "DeviceMountPaths" ).toString());
        QString text1 = mountPath.isEmpty() ? devPath : mountPath;
        return new DeviceItem( parentItem, QStringList() << text1 << text1 << "Devices" << devPath, view, !mountPath.isEmpty());
    }
    void setMounted(bool mount)
    {
        const QString devName = QFileInfo( m_devPath ).fileName();
        static QString fsMount[2] = { "FilesystemUnmount", "FilesystemMount" };
        QDBusMessage m = methodCall(devName, fsMount[mount]);
        if (mount)
            m << QString(); //filesystem
        m << QStringList(); //options
        QDBusMessage reply = QDBusConnection::systemBus().call( m );
        if (!reply.errorMessage().isEmpty())
            QMessageBox::critical(m_mainWin, QString("Udisks error:"), reply.errorMessage());
    }
    void changeState()
    {
        m_mountPath = DbusCalls::deviceInfo( m_devPath, "DeviceMountPaths" ).toString();
        m_mounted = !m_mountPath.isEmpty();
        if (m_mounted)
        {
            for (int i = 0; i < 2; ++i)
                setText(i, m_mountPath);
            int t = 0;
            setToolTip(0, QString::number(realSize((float)m_freeBytes, &t)) + spaceType[t] + " Free");
        }
        else
        {
            int t = 0;
            QString size = QString::number( realSize( DbusCalls::deviceInfo(m_devPath, "PartitionSize").toFloat(), &t ) );
            int i = size.mid( size.indexOf(".")+3, 1).toInt();
            if ( i > 4 ) //if we have a higher value then 4 we round up....
                size.replace(size.indexOf(".")+2, 1, QString::number(size.mid( size.indexOf(".")+2, 1).toInt()+1));
            size.chop( size.length() - (size.indexOf(".")+3) );
            setText(0, size + spaceType[t] + " Device");
        }
        m_tb->setIcon(mountIcon(m_mounted, 16, m_view->palette().color(m_view->foregroundRole())));
        m_tb->setToolTip( m_mounted ? "Unmount" : "Mount" );
    }
    bool isMounted() { return m_mounted; }
    QString mountPath() { return m_mountPath; }
    QString devPath() { return m_devPath; }
    quint64 usedBytes() { return m_usedBytes; }
    quint64 freeBytes() { return m_freeBytes; }
    quint64 totalBytes() { return m_totalBytes; }
    int used() { return m_usedBytes ? (int)(((float)m_usedBytes/(float)m_totalBytes)*100) : 0; }
public slots:
    void mount() { setMounted(true); }
    void unMount() { setMounted(false); }
    void toggleMount() { setMounted(!isMounted()); }
signals:
    void usageChanged( QTreeWidgetItem *item );
private slots:
    void updateSpace()
    {
        if ( m_mounted )
        {
            if ( m_usedBytes != Operations::getDriveInfo(m_mountPath, Operations::Used) )
            {
                m_usedBytes = Operations::getDriveInfo(m_mountPath, Operations::Used);
                emit usageChanged( this );
            }
            if ( m_freeBytes != Operations::getDriveInfo(m_mountPath, Operations::Free) )
            {
                m_freeBytes = Operations::getDriveInfo(m_mountPath, Operations::Free);
                int t = 0;
                QString free(QString::number(realSize((float)m_freeBytes, &t)));
                setToolTip(0, free + spaceType[t] + " Free");
            }
            if ( m_totalBytes != Operations::getDriveInfo(m_mountPath, Operations::Total) )
                m_totalBytes = Operations::getDriveInfo(m_mountPath, Operations::Total);
        }
    }
    void viewEvent()
    {
        const QRect &r(m_view->visualItemRect(this));
        int y = r.y()+((r.height()-m_tb->height())/2), x = r.x()+8;
        m_tb->setVisible(m_parentItem->isExpanded());
        m_tb->move(x, y);
    }

private:
    bool m_mounted;
    QString m_mountPath, m_devPath;
    QTreeWidget *m_view;
    QMainWindow *m_mainWin;
    QToolButton *m_tb;
    QTreeWidgetItem *m_parentItem;
    quint64 m_usedBytes, m_freeBytes, m_totalBytes;
    QTimer *m_timer;
    friend class DeviceManager;
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef QMap<QString, DeviceItem*> DeviceItems;
    explicit DeviceManager( QObject *parent = 0 );
    static DeviceManager *manage(QTreeWidget *tw);
    DeviceItems devices() { return m_items; }
    static DeviceItem *deviceItemForFile( const QString &file );
signals:
    void usageChanged(QTreeWidgetItem *item);
private slots:
    void deviceAdded( const QDBusObjectPath &device );
    void deviceRemoved( const QDBusObjectPath &device );
    void deviceChanged( const QDBusObjectPath &device );
    void populateLater();
private:
    DeviceItems m_items;
    QTreeWidget *m_tree;
    QTreeWidgetItem *m_devicesParent;
    friend class DeviceItem;
};

}

#endif // DEVICEMANAGER_H
