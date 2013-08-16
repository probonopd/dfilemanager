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

#include <QTreeWidget>
#include <QtXml/QDomDocument>
#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QSettings>

#include <QMainWindow>
#include "operations.h"
#include "devicemanager.h"

namespace DFM
{
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
             return  QSize(-1, QFontMetrics(option.font).height()*3/2);
    }
    inline bool isHeader( const QModelIndex &index ) const { return !index.parent().isValid(); }
private:
    PlacesView *m_placesView;
    friend class PlacesView;
};

class PlacesView : public QTreeWidget
{
    Q_OBJECT
public:
    enum Role{ Name = 0, Path = 1, Icon = 2};
    PlacesView( QWidget *parent );
    QMenu *containerAsMenu( const int &cont );
    DeviceManager *deviceManager() { return m_devManager; }
    void populate();

public slots:
    void renPlace();
    void addPlace(QString name, QString path, QIcon icon, QTreeWidgetItem *parent = 0, const bool &storeSettings = true);
    void addPlaceCont();
    void setPlaceIcon();
    void removePlace();
    void activateAppropriatePlace(const QString &index);
    void store();

protected:
    virtual void dropEvent(QDropEvent *event);
    inline QStringList mimeTypes() const { return QStringList() << "text/uri-list"; }
    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const;
    void contextMenuEvent(QContextMenuEvent *);
    void keyPressEvent(QKeyEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:
    void newTabRequest(QString path);

private:
    QTreeWidgetItem *m_lastClicked;
    QMainWindow *m_mainWindow;
    DeviceManager *m_devManager;
    friend class PlacesViewDelegate;
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
