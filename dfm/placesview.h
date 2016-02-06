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
#include <QAction>
#include <QMainWindow>


#include "widgets.h"
#include "operations.h"
#include "devices.h"

namespace DFM
{

class Place : public QStandardItem
{
public:
    enum Info { Name = 0, Path = 1, Icon = 2 };
    inline Place(const QString &name = QString(), const QString &path = QString(), const QString &iconName = QString(), const QIcon &icon = QIcon())
    {
        m_values << name << path << iconName;
        m_icon = icon;
    }
    inline Place(const QStringList &texts = QStringList(), const QIcon &icon = QIcon()) : m_values(texts) { while (m_values.count() < 3) m_values << QString(); m_icon = icon; }
    inline Place(Place *p) { m_values << p->name() << p->path() << p->iconName(); m_icon = p->fileIcon(); }
    ~Place(){}
    virtual QStringList values() { return m_values; }
    virtual QString name() const { return m_values[Name]; }
    virtual QString path() const { return m_values[Path]; }
    virtual QString iconName() const { return m_values[Icon]; }
    virtual void setName(const QString &name) { m_values[Name] = name; }
    virtual void setPath(const QString &path) { m_values[Path] = path; }
    virtual void setIconName(const QString &iconName) { m_values[Icon] = iconName; }
    inline QIcon fileIcon() { return m_icon; }
    inline void setFileIcon(const QIcon &icon) { m_icon = icon; }

private:
    QStringList m_values;
    QIcon m_icon;
};

class Container : public Place
{
public:
    typedef QList<Place *> Places;
    inline Container(const QString &name) : Place(QStringList() << name) { setSelectable(false); }
    inline Container(Container *c) : Place(c)
    {
        foreach (Place *p, c->places())
            appendRow(new Place(p));
        setSelectable(false);
    }
    ~Container(){}
    inline Place *place(const int i) { return static_cast<Place *>(child(i)); }
    inline Places places()
    {
        Places p;
        for (int i = 0; i < rowCount(); ++i)
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
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
    {
        QSize sz(QStyledItemDelegate::sizeHint(option, index));
         if (!isHeader(index))
             return sz;
         else
             return  sz + QSize(0, option.fontMetrics.height()/2);
    }
    inline bool isHeader(const QModelIndex &index) const { return !index.parent().isValid(); }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

private:
    PlacesView *m_placesView;
};

class DeviceManager;
class Button;
class DeviceItem : public QObject, public Place //have to inherit QObject for signals/slots/eventHandling
{
    Q_OBJECT
public:
    DeviceItem(DeviceManager *parentItem = 0, PlacesView *view = 0, Device *dev = 0);
    ~DeviceItem();
    inline void setMounted(const bool mount) { if (m_device) m_device->setMounted(mount); }
    inline bool isMounted() const { return m_device?m_device->isMounted():false; }
    inline QString mountPath() const { return m_device?m_device->mountPath():QString(); }
    inline QString devPath() const { return m_device?m_device->devPath():QString(); }
    inline QString product() const { return m_device?m_device->product():QString(); }
    inline quint64 usedBytes() const { return m_device?m_device->usedBytes():0; }
    inline quint64 freeBytes() const { return m_device?m_device->freeBytes():0; }
    inline quint64 totalBytes() const { return m_device?m_device->totalBytes():0; }
    inline int used() const { return m_device?m_device->used():0; }
    inline bool isVisible() const { return m_isVisible; }
    bool isHidden() const;
    inline QList<QAction *> actions() const { return m_actions; }
    QString name() const { return isMounted() ? mountPath() : m_device?m_device->description():QString(); }
    QString path() const { return mountPath(); }
//    inline void mount() { setMounted(true); }
//    inline void unMount() { setMounted(false); }

public slots:
    void updateTb();
    void setVisible(bool v);
    void setHidden();
    void hide();
    void show();

private slots:
    inline void toggleMount() { setMounted(!isMounted()); }
    void updateSpace();
    void changeState();
    void updateIcon();
    inline void buttonDestroyed() { m_button = 0; }

private:
    bool m_isVisible, m_isHidden;
    PlacesView *m_view;
    Button *m_button;
    DeviceManager *m_manager;
    QTimer *m_timer;
    Device *m_device;
    QList<QAction *> m_actions;
};

class DeviceManager : public QObject, public Container
{
    Q_OBJECT
public:
    DeviceManager(const QStringList &texts, QObject *parent = 0);
    ~DeviceManager();
    inline QMap<Device *, DeviceItem *> deviceItems() { return m_items; }
    DeviceItem *deviceItemForFile(const QString &file);
    inline bool isDevice(const QStandardItem *item) { return (bool)(item->parent() == this); }
    inline QList<QAction *> actions() const { return m_actions; }

private slots:
    void addDevs();
    void devAdded(Device *dev);
    void devRemoved(Device *dev);
    void showHiddenDevices();

private:
    PlacesView *m_view;
    QMap<Device *, DeviceItem *> m_items;
    QList<QAction *> m_actions;
};

class PlacesModel : public QStandardItemModel
{
public:
    inline explicit PlacesModel(QObject *parent = 0) : QStandardItemModel(parent), m_places(qobject_cast<PlacesView *>(parent)) {}
    ~PlacesModel();

    QVariant data(const QModelIndex &index, int role) const;
    QStringList mimeTypes() const { return QStringList() << "text/uri-list"; }

private:
    PlacesView *m_places;
};

class PlacesView : public QTreeView
{
    Q_OBJECT
public:
    typedef QList<Container *> Containers;
    PlacesView(QWidget *parent);
    ~PlacesView();
    QMenu *containerAsMenu(const int cont);
    bool getKdePlaces();
    bool hasPlace(const QString &path) const;
    inline QStringList hiddenDevices() const { return m_hiddenDevices; }
    inline void addHiddenDevice(const QString &devPath) { if (!m_hiddenDevices.contains(devPath)) m_hiddenDevices << devPath; }
    inline void removeHiddenDevice(const QString &devPath) { m_hiddenDevices.removeOne(devPath); }
    inline DeviceManager *deviceManager() { return m_devManager; }
    inline QModelIndex indexFromItem(QStandardItem *item) const { return m_model->indexFromItem(item); }

    inline QStandardItem *itemFromIndex(const QModelIndex &index) const { return m_model->itemFromIndex(index); }
    inline QStandardItem *itemAt(const QPoint &pos) { return itemFromIndex(indexAt(pos)); }
    inline QStandardItem *currentItem() const { return itemFromIndex(currentIndex()); }

    template<typename T>
    inline T itemFromIndex(const QModelIndex &index) const { return dynamic_cast<T>(m_model->itemFromIndex(index)); }
    template<typename T>
    inline T itemAt(const QPoint &pos) { return itemFromIndex<T>(indexAt(pos)); }
    template<typename T>
    inline T currentItem() const { return itemFromIndex<T>(currentIndex()); }

    inline Container *container(const int i) const { return dynamic_cast<Container *>(m_model->item(i)); }
    inline void setCurrentItem(QStandardItem *item) { setCurrentIndex(indexFromItem(item)); }
    inline int containerCount() const { return m_model->rowCount(); }
    inline Containers containers() const
    {
        Containers c;
        for (int i = 0; i < containerCount(); ++i)
            if (container(i) != (Container*)m_devManager)
                c << container(i);
        return c;
    }

    void showHiddenDevices();
    void hideHiddenDevices();

public slots:
    void renPlace();
    void addPlace(QString name, QString path, QIcon icon, QStandardItem *parent = 0, const bool storeSettings = true);
    void addPlaceCont();
    void setPlaceIcon();
    void removePlace();
    void activateAppropriatePlace(const QString &path);
    void populate();
    void store();
    void updateAllWindows();
    void paletteOps();

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;
    void dropEvent(QDropEvent *event);
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paletteChange(const QPalette &pal);
    void paintEvent(QPaintEvent *event);
    void drawItemsRecursive(QPainter *painter, const QRect &clip, const QModelIndex &parent = QModelIndex());

signals:
    void newTabRequest(const QUrl &url);
    void placeActivated(const QUrl &url);
    void changed();

private slots:
    void emitPath(const QModelIndex &index);

private:
    QStandardItem *m_lastClicked;
    PlacesModel *m_model;
    DeviceManager *m_devManager;
    QTimer *m_timer;
    QStringList m_hiddenDevices;
};

}

#endif // PLACESVIEW_H
