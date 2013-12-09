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


#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QSharedMemory>
#include <QFileSystemWatcher>
#include <QFile>
#include <QDir>
#include <QMap>
#if 0
#include <QMainWindow>
#include "dockwidget.h"
#include "placesview.h"
#include "config.h"
#endif

#ifdef Q_WS_X11
#include <QX11Info>
#endif

#include "interfaces.h"

//#include <GL/glut.h>

#define APP static_cast<Application*>(QApplication::instance())
#define MAINWINDOW static_cast<DFM::MainWindow*>(APP->mainWindow())
#define DPY QX11Info::display()

class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char *argv[], const QString &key = "dfmkey"); /*: QApplication(argc, argv) {}*/
    inline bool isRunning() { return m_isRunning; }
    bool setMessage(const QStringList &message);
    QList<ThumbInterface *> thumbIfaces() { return m_allThumbIfaces; }
    QList<ThumbInterface *> activeThumbIfaces() { return m_thumbIfaces.values(); }
    inline bool hasThumbIfaces() { return !m_allThumbIfaces.isEmpty(); }
    inline bool isActive(ThumbInterface *ti) { return m_thumbIfaces.contains(ti->name()); }
    inline void activateThumbInterface(ThumbInterface *ti) { m_thumbIfaces.insert(ti->name(), ti); }
    void activateThumbInterface(const QString &name);
    inline void deActivateThumbInterface(ThumbInterface *ti) { m_thumbIfaces.remove(ti->name()); }
    void deActivateThumbInterface(const QString &name);
    void loadPlugins();
    void loadPluginsFromDir(const QDir dir);
//    bool notify(QObject * receiver, QEvent * event);

signals:
    void lastMessage( const QStringList &message );

private slots:
    void fileChanged(const QString &file);

#if 0
    inline void manageDock( DFM::Docks::DockWidget *dock ) { m_docks << dock; }
    inline void setMainWindow( QMainWindow *mainWin ) { m_mainWindow = mainWin; }
    inline QMainWindow *mainWindow() { return m_mainWindow; }
    inline void setPlacesView( DFM::PlacesView *places ) { m_places = places; }
    inline DFM::PlacesView *placesView() { return m_places; }
#endif
#ifdef Q_WS_X11
#if 0
protected:
    bool x11EventFilter(XEvent *xe);
#endif
#endif

private:
    bool m_isRunning;
    QSharedMemory *m_sharedMem;
    QFileSystemWatcher *m_fsWatcher;
    QString m_key;
    QString m_filePath;
    QFile m_file;
    QMap<QString, ThumbInterface *> m_thumbIfaces;
    QList<ThumbInterface *> m_allThumbIfaces;
#if 0
    QList<DFM::Docks::DockWidget *> m_docks;
    QMainWindow *m_mainWindow;
    DFM::PlacesView *m_places;
#endif
};

#endif // APPLICATION_H
