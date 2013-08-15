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
#include "viewanimator.h"
#include "mainwindow.h"
#ifdef Q_WS_X11

using namespace DFM;

DeviceItem::DeviceItem(QTreeWidgetItem *parentItem, QTreeWidget *view, Solid::Device solid )
    : QTreeWidgetItem( parentItem )
    , m_view(view)
    , m_mainWin(MainWindow::currentWindow())
    , m_parentItem(parentItem)
    , m_tb(new QToolButton(m_view))
    , m_timer(new QTimer(this))
    , m_solid(solid)
{
    m_tb->setIcon(mountIcon(isMounted(), 16, m_view->palette().color(m_view->foregroundRole())));
    m_tb->setVisible(true);
    m_tb->setFixedSize(16, 16);
    m_tb->setToolTip( isMounted() ? "Unmount" : "Mount" );
    connect (m_tb, SIGNAL(clicked()), this, SLOT(toggleMount()));
    connect (m_timer, SIGNAL(timeout()), this, SLOT(updateSpace()));
    m_timer->start(1000);
    for ( int i = 2; i<4; ++i )
        setText( i, "Devices" );
    if ( isMounted() )
    {
        setText(PlacesView::Path, mountPath());
        setText(PlacesView::Name, mountPath());
    }
    else
    {
        setText(PlacesView::Name, m_solid.description());
    }
    connect (m_solid.as<Solid::StorageAccess>(), SIGNAL(accessibilityChanged(bool, const QString &)), this, SLOT(changeState()));
    updateSpace();
    viewEvent();

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

void
DeviceItem::setMounted( const bool &mount )
{
    if ( !m_solid.isValid() )
        return;

    if ( mount )
        m_solid.as<Solid::StorageAccess>()->setup();
    else
        m_solid.as<Solid::StorageAccess>()->teardown();
}

void
DeviceItem::updateSpace()
{
    if ( isMounted() )
    {
        if ( Configuration::config.behaviour.devUsage )
            m_view->update( m_view->indexAt(m_view->visualItemRect(this).center()) );
        int t = 0;
        QString free(QString::number(realSize((float)freeBytes(), &t)));
        setToolTip(0, free + spaceType[t] + " Free");
    }
}

void
DeviceItem::changeState()
{
    if (isMounted())
    {
        for (int i = 0; i < 2; ++i)
            setText(i, mountPath());
        int t = 0;
        setToolTip(0, QString::number(realSize((float)freeBytes(), &t)) + spaceType[t] + " Free");
    }
    else
    {
        setText(PlacesView::Name, m_solid.description());
    }
    m_tb->setIcon(mountIcon(isMounted(), 16, m_view->palette().color(m_view->foregroundRole())));
    m_tb->setToolTip( isMounted() ? "Unmount" : "Mount" );
}

void
DeviceItem::viewEvent()
{
    const QRect &r(m_view->visualItemRect(this));
    int y = r.y()+((r.height()-m_tb->height())/2), x = r.x()+8;
    m_tb->setVisible(m_parentItem->isExpanded());
    m_tb->move(x, y);
}

DeviceManager *DeviceManager::m_instance = 0;
QTreeWidgetItem *DeviceManager::m_devicesParent = 0;

DeviceManager::DeviceManager(QObject *parent) : QObject(parent),
    m_tree(static_cast<QTreeWidget*>(parent))
{
    QTimer::singleShot(200, this, SLOT(populateLater()));

    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)), this, SLOT(deviceAdded(QString)));
    connect (Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)), this, SLOT(deviceRemoved(QString)));
}

void
DeviceManager::deviceAdded(const QString &dev)
{
    Solid::Device device = Solid::Device(dev);
    if ( device.is<Solid::StorageAccess>() )
        m_items.insert(dev, new DeviceItem(m_devicesParent, m_tree, device));
}

void
DeviceManager::deviceRemoved(const QString &dev)
{
    if ( m_items.contains(dev) )
        delete m_items.take(dev);
}

DeviceManager
*DeviceManager::manage(QTreeWidget *tw)
{
    if (!m_instance)
        m_instance = new DeviceManager(tw);
    return m_instance;
}

void
DeviceManager::populateLater()
{
    m_devicesParent = new QTreeWidgetItem(m_tree);
    for (int i = 0; i<4; ++i)
        m_devicesParent->setText( i, "Devices" );

    m_devicesParent->setExpanded(true);

    foreach ( Solid::Device dev, Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess) )
        m_items.insert(dev.udi(), new DeviceItem(m_devicesParent, m_tree, dev ));
}

DeviceItem
*DeviceManager::deviceItemForFile(const QString &file)
{
    QString s(file.isEmpty() ? "/" : file);
    foreach ( DeviceItem *item, DeviceManager::devices().values() )
        if ( s == item->mountPath() )
            return item;
    return deviceItemForFile(s.mid(0, s.lastIndexOf("/")));
}

#endif
