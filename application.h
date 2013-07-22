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
#include <QMainWindow>
#include "dockwidget.h"
#include "placesview.h"
#include "config.h"

#ifdef Q_WS_X11
#include <QX11Info>
#endif

//#include <GL/glut.h>

#define APP static_cast<Application*>(QApplication::instance())
#define MAINWINDOW static_cast<DFM::MainWindow*>(APP->mainWindow())
#define DPY QX11Info::display()

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv, int = ApplicationFlags);
#ifdef Q_WS_X11
    inline void manageDock( DFM::Docks::DockWidget *dock ) { m_docks << dock; }
    inline void setMainWindow( QMainWindow *mainWin ) { m_mainWindow = mainWin; }
    inline QMainWindow *mainWindow() { return m_mainWindow; }
    inline void setPlacesView( DFM::PlacesView *places ) { m_places = places; }
    inline DFM::PlacesView *placesView() { return m_places; }
signals:
    void paletteChanged( QPalette newPal );
protected:
    bool x11EventFilter(XEvent *xe);
//    inline bool event(QEvent *e) { qDebug() << e; return QApplication::event(e); }
private:
    QList<DFM::Docks::DockWidget *> m_docks;
    QMainWindow *m_mainWindow;
    DFM::PlacesView *m_places;
//    static Atom m_netWmDesktop, m_netClientListStacking;
#endif
};

#endif // APPLICATION_H
