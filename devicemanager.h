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

#include <QMainWindow>
#include <QToolButton>
#include <QMessageBox>
#include "application.h"
#include "operations.h"
#include "iconprovider.h"

#include "solid/device.h"
#include "solid/block.h"
#include "solid/storagedrive.h"
#include "solid/storagevolume.h"
#include "solid/storageaccess.h"
#include "solid/devicenotifier.h"

namespace DFM
{

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

static inline QPixmap mountIcon( const bool &mounted, const int &size, const QColor &col )
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
    DeviceItem( QTreeWidgetItem *parentItem = 0, QTreeWidget *view = 0, Solid::Device solid = Solid::Device() );
    ~DeviceItem() { m_tb->deleteLater(); }
    void setMounted( const bool &mount );
    inline bool isMounted() const { return m_solid.isValid() && m_solid.as<Solid::StorageAccess>()->isAccessible(); }
    inline QString mountPath() const { return m_solid.isValid() ? m_solid.as<Solid::StorageAccess>()->filePath() : QString(); }
    inline QString devPath() const { return m_solid.isValid() ? m_solid.as<Solid::Block>()->device() : QString(); }
    inline quint64 usedBytes() const { return Operations::getDriveInfo(mountPath(), Operations::Used); }
    inline quint64 freeBytes() const { return Operations::getDriveInfo(mountPath(), Operations::Free); }
    inline quint64 totalBytes() const { return m_solid.isValid() ? m_solid.as<Solid::StorageVolume>()->size() : 0L; }
    inline int used() const { return usedBytes() ? (int)(((float)usedBytes()/(float)totalBytes())*100) : 0; }
//    inline void mount() { setMounted(true); }
//    inline void unMount() { setMounted(false); }

private slots:
    inline void toggleMount() { setMounted(!isMounted()); }
    void updateSpace();
    void viewEvent();
    void changeState();

private:
    QTreeWidget *m_view;
    QMainWindow *m_mainWin;
    QToolButton *m_tb;
    QTreeWidgetItem *m_parentItem;
    QTimer *m_timer;
    Solid::Device m_solid;
    friend class DeviceManager;
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef QMap<QString, DeviceItem*> DeviceItems;
    explicit DeviceManager( QObject *parent = 0 );
    static DeviceManager *manage(QTreeWidget *tw);
    inline DeviceItems deviceItems() { return m_items; }
    static inline DeviceItems devices() { return m_instance->deviceItems(); }
    static DeviceItem *deviceItemForFile( const QString &file );
    static inline QTreeWidgetItem *devicesParent() { return m_devicesParent; }
    static inline bool itemIsDevice( const QTreeWidgetItem *item ) { return item == m_devicesParent || ( item->parent() && item->parent() == m_devicesParent ); }
private slots:
    void populateLater();
    void deviceAdded( const QString &dev );
    void deviceRemoved( const QString &dev );
private:
    static DeviceManager *m_instance;
    DeviceItems m_items;
    QTreeWidget *m_tree;
    static QTreeWidgetItem *m_devicesParent;
    friend class DeviceItem;
};

}

#endif // DEVICEMANAGER_H
