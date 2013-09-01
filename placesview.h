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


#ifndef PLACESVIEW_H
#define PLACESVIEW_H

#include <QTreeView>
#include <QtXml/QDomDocument>
#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QToolButton>
#include <QPainter>
#include <QDebug>

#include <QMainWindow>
#include "operations.h"

#ifdef Q_WS_X11
#include "solid/device.h"
#include "solid/block.h"
#include "solid/storagedrive.h"
#include "solid/storagevolume.h"
#include "solid/storageaccess.h"
#include "solid/devicenotifier.h"
#endif

namespace DFM
{

class Place : public QStandardItem
{
public:
    enum Info { Name = 0, Path = 1, Icon = 2 };
    inline Place(const QString &name = QString(), const QString &path = QString(), const QString &iconName = QString())
    {
        m_values << name << path << iconName; ;
    }
    inline Place(const QStringList &texts = QStringList()) : m_values(texts) { while (m_values.count() < 3) m_values << QString(); }
    inline Place( Place *p ) { m_values << p->name() << p->path() << p->iconName(); }
    virtual QStringList values() { return m_values; }
    virtual QString name() const { return m_values[Name]; }
    virtual QString path() const { return m_values[Path]; }
    virtual QString iconName() const { return m_values[Icon]; }
    virtual void setName( const QString &name ) { m_values[Name] = name; }
    virtual void setPath( const QString &path ) { m_values[Path] = path; }
    virtual void setIconName( const QString &iconName ) { m_values[Icon] = iconName; }

private:
    QStringList m_values;
};

class Container : public Place
{
public:
    typedef QList<Place *> Places;
    Container( const QString &name ) : Place(QStringList() << name) {}
    Container( Container *c ) : Place( c )
    {
        foreach ( Place *p, c->places() )
            appendRow( new Place(p) );
    }
    inline Place *place( const int &i ) { return static_cast<Place *>(child(i)); }
    inline Places places()
    {
        Places p;
        for ( int i = 0; i < rowCount(); ++i )
            p << place(i);
        return p;
    }
};

class PlacesView;

class PlacesViewDelegate : public QStyledItemDelegate
{
public:
    inline explicit PlacesViewDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent), m_placesView(qobject_cast<PlacesView *>(parent)) {}
protected:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex & index ) const
    {
         if ( !isHeader(index) )
         {
             return QSize(-1, option.decorationSize.height()+2);
         }
         else
             return  QSize(-1, option.fontMetrics.height()*3/2);
    }
    inline bool isHeader( const QModelIndex &index ) const { return !index.parent().isValid(); }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

private:
    PlacesView *m_placesView;
};

#ifdef Q_WS_X11
class DeviceManager;
class DeviceItem : public QObject, public Place //have to inherit QObject for signals/slots/eventHandling
{
    Q_OBJECT
public:
    DeviceItem( DeviceManager *parentItem = 0, PlacesView *view = 0, Solid::Device solid = Solid::Device() );
    ~DeviceItem() { m_tb->deleteLater(); }
    void setMounted( const bool &mount );
    inline bool isMounted() const { return m_solid.isValid() && m_solid.as<Solid::StorageAccess>()->isAccessible(); }
    inline QString mountPath() const { return m_solid.isValid() ? m_solid.as<Solid::StorageAccess>()->filePath() : QString(); }
    inline QString devPath() const { return m_solid.isValid() ? m_solid.as<Solid::Block>()->device() : QString(); }
    inline quint64 usedBytes() const { return Operations::getDriveInfo<Operations::Used>(mountPath()); }
    inline quint64 freeBytes() const { return Operations::getDriveInfo<Operations::Free>(mountPath()); }
    inline quint64 totalBytes() const { return Operations::getDriveInfo<Operations::Total>(mountPath()); }
    inline int used() const { return usedBytes() ? (int)(((float)usedBytes()/(float)totalBytes())*100) : 0; }
    QString name() const { return isMounted() ? mountPath() : m_solid.description(); }
    QString path() const { return mountPath(); }
//    inline void mount() { setMounted(true); }
//    inline void unMount() { setMounted(false); }

public slots:
    void updateTb();

private slots:
    inline void toggleMount() { setMounted(!isMounted()); }
    void updateSpace();
    void changeState();

private:
    PlacesView *m_view;
    QToolButton *m_tb;
    DeviceManager *m_manager;
    QTimer *m_timer;
    Solid::Device m_solid;
};

class DeviceManager : public QObject, public Container
{
    Q_OBJECT
public:
    DeviceManager( const QStringList &texts, QObject *parent = 0 );
    typedef QMap<QString, DeviceItem*> DeviceItems;
    inline DeviceItems deviceItems() { return m_items; }
    DeviceItem *deviceItemForFile( const QString &file );
    inline bool isDevice( const QStandardItem *item ) { return (bool)(item->parent() == this); }

private slots:
    void populate();
    void deviceAdded( const QString &dev );
    void deviceRemoved( const QString &dev );

private:
    PlacesView *m_view;
    DeviceItems m_items;
};

#endif

class PlacesModel : public QStandardItemModel
{
public:
    inline explicit PlacesModel( QObject *parent = 0 ) : QStandardItemModel( parent ), m_places(qobject_cast<PlacesView *>(parent)) {}
protected:
    QVariant data(const QModelIndex &index, int role) const;
    inline QStringList mimeTypes() const { return QStringList() << "text/uri-list"; }
private:
    PlacesView *m_places;
};

class PlacesView : public QTreeView
{
    Q_OBJECT
public:
    typedef QList<Container *> Containers;
    PlacesView( QWidget *parent );
    QMenu *containerAsMenu( const int &cont );
    inline DeviceManager *deviceManager() { return m_devManager; }
    inline QModelIndex indexFromItem( QStandardItem *item ) const { return m_model->indexFromItem(item); }

    inline QStandardItem *itemFromIndex( const QModelIndex &index ) const { return m_model->itemFromIndex(index); }
    inline QStandardItem *itemAt( const QPoint &pos ) { return itemFromIndex(indexAt(pos)); }
    inline QStandardItem *currentItem() const { return itemFromIndex(currentIndex()); }

    template<typename T>
    inline T itemFromIndex( const QModelIndex &index ) const { return dynamic_cast<T>(m_model->itemFromIndex(index)); }
    template<typename T>
    inline T itemAt( const QPoint &pos ) { return itemFromIndex<T>(indexAt(pos)); }
    template<typename T>
    inline T currentItem() const { return itemFromIndex<T>(currentIndex()); }

    inline Container *container( const int &i ) { return dynamic_cast<Container *>(m_model->item(i)); }
    inline void setCurrentItem(QStandardItem *item) { setCurrentIndex(indexFromItem(item)); }
    inline int containerCount() { return m_model->rowCount(); }
    inline Containers containers()
    {
        Containers c;
        for ( int i = 0; i < containerCount(); ++i )
            if ( container(i) != (Container*)m_devManager )
                c << container(i);
        return c;
    }

public slots:
    void renPlace();
    void addPlace(QString name, QString path, QIcon icon, QStandardItem *parent = 0, const bool &storeSettings = true);
    void addPlaceCont();
    void setPlaceIcon();
    void removePlace();
    void activateAppropriatePlace(const QString &path);
    void populate();
    void store();
    void updateAllWindows();

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;
    void dropEvent(QDropEvent *event);
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:
    void newTabRequest( const QString &path );
    void placeActivated( const QString &path );
    void changed();

private slots:
    void emitPath(const QModelIndex &index);

private:
    QStandardItem *m_lastClicked;
    PlacesModel *m_model;
    DeviceManager *m_devManager;
    QTimer *m_timer;
};

}

#if 0
        system("df -kTh > /tmp/dfm_devices");
        QStringList devices;
        QFile devList("/tmp/dfm_devices");
        devList.open(QIODevice::ReadOnly);
        QTextStream readData(&devList);

        while (!readData.atEnd())
            devices << readData.readLine();

        devList.close();

        QString used, deviceName;


        foreach(QString deviceString, devices)
            if(deviceString[0] == '/')
            {
                used = deviceString.split(QRegExp("\\s+")).at(5);
                used.remove("%");
                deviceName = deviceString.split(QRegExp("\\s+")).at(6);
                du.insert(deviceName,used.toInt());
            }
        return du;
#endif

#endif // PLACESVIEW_H
