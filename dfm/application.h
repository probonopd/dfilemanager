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
#include <QMap>
#include <QList>
#include "globals.h"

#define dApp static_cast<Application*>(QApplication::instance())
//#define DPY QX11Info::display()
#define SEP "#PATHSEP#"
#define CSEP "#SEPARATOR#"

class ThumbInterface;
class QDir;
class QLocalSocket;
class QLocalServer;
class QString;

class Application : public QApplication
{
    Q_OBJECT
public:
    enum Type { Browser = 0, IOJob = 1 };
    Application(int &argc, char *argv[]);
    inline bool isRunning() { return m_isRunning; }
    void setMessage(QStringList message, const QString &serverName = QString());
    QList<ThumbInterface *> thumbIfaces();
    QList<ThumbInterface *> activeThumbIfaces();
    bool hasThumbIfaces();
    bool isActive(ThumbInterface *ti);
    void activateThumbInterface(ThumbInterface *ti);
    void activateThumbInterface(const QString &name);
    void deActivateThumbInterface(ThumbInterface *ti);
    void deActivateThumbInterface(const QString &name);
    inline Type appType() { return m_type; }

    void loadPlugins();
    void loadPluginsFromDir(const QDir &dir);

//    bool notify(QObject * receiver, QEvent * event);

signals:
    void lastMessage(const QStringList &message);

private slots:
    void newServerConnection();
    void socketConnected();

#if defined(HASX11)
#if 0
    inline void manageDock(DFM::Docks::DockWidget *dock) { m_docks << dock; }
    inline void setMainWindow(QMainWindow *mainWin) { m_mainWindow = mainWin; }
    inline QMainWindow *mainWindow() { return m_mainWindow; }
    inline void setPlacesView(DFM::PlacesView *places) { m_places = places; }
    inline DFM::PlacesView *placesView() { return m_places; }

protected:
    bool x11EventFilter(XEvent *xe);
#endif
#endif

private:
    Type m_type;
    bool m_isRunning;
    QString m_key, m_message;
    QMap<QString, ThumbInterface *> m_thumbIfaces;
    QList<ThumbInterface *> m_allThumbIfaces;
    DFM::IOJobData m_ioJobData;
    QLocalSocket *m_socket;
    QLocalServer *m_server;
#if 0
    QList<DFM::Docks::DockWidget *> m_docks;
    QMainWindow *m_mainWindow;
    DFM::PlacesView *m_places;
#endif
};

#endif // APPLICATION_H
