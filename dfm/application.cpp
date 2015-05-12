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


#include "application.h"
//#include "mainwindow.h"
#include "config.h"
#include <QSharedMemory>
#include <QDebug>
#include <QDesktopServices>
#include <QPluginLoader>
#include <QLocalServer>
#include <QLocalSocket>
#include <QFile>
#include <QDir>

//#if defined(HASX11)
//#include <QX11Info>
//#endif

#include "interfaces.h"

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

#if defined(HASX11)
#if 0
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/X.h>
static Atom netWmDesktop = 0;
static Atom netClientListStacking = 0;
#endif
#endif

static QString name[] = { "dfm_browser", "dfm_iojob" };

Application::Application(int &argc, char *argv[]) : QApplication(argc, argv)
{
    if (arguments().contains("--iojob"))
        m_type = IOJob;
    else
        m_type = Browser;

    m_message = "--";
    m_server = new QLocalServer(this);
    m_socket = new QLocalSocket(this);

    const QString &key = name[m_type];
    m_socket->connectToServer(key);
    if (m_socket->error() == QLocalSocket::ConnectionRefusedError) //we are assuming a crash happened...
        m_server->removeServer(key);
    m_isRunning = m_socket->state() == QLocalSocket::ConnectedState && m_socket->error() != QLocalSocket::ConnectionRefusedError;
    DFM::Store::readConfig();
    if (m_type == IOJob && !DFM::Store::config.behaviour.useIOQueue)
        return;
//    m_socket->disconnectFromServer();

    connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(m_server, SIGNAL(newConnection()), this, SLOT(newServerConnection()));

    if (!m_isRunning)
    {
        if (!m_server->listen(name[m_type]))
            qDebug() << "Wasnt able to start the localServer... IPC will not work right";
        setOrganizationName("dfm");
    }
    else
        qDebug() << "dfm is already running";
#if defined(HASX11)
#if 0
    netWmDesktop = XInternAtom(DPY, "_NET_WM_DESKTOP", False);
    netClientListStacking = XInternAtom(DPY, "_NET_CLIENT_LIST_STACKING", False);
#endif
#endif
}

//QLocalSocket::connected() [SIGNAL]
void
Application::socketConnected()
{
    m_socket->write(m_message.toLatin1().data(), m_message.size());
    m_socket->flush();
}

//QLocalServer::newConnection() [SIGNAL]
void
Application::newServerConnection()
{
    QLocalSocket *ls = m_server->nextPendingConnection();
    ls->waitForReadyRead();
    const QString message(ls->readAll());
    connect(ls, SIGNAL(disconnected()), ls, SLOT(deleteLater()));
    ls->disconnectFromServer();
    emit lastMessage(message.split(CSEP));
}

void
Application::setMessage(QStringList message, const QString &serverName)
{
    m_socket->abort();
    const QString &server = serverName.isEmpty() ? name[m_type] : serverName;
    m_message = message.join(CSEP);
    m_socket->connectToServer(server);
}


void
Application::loadPlugins()
{
#if defined(ISUNIX)
#if defined(INSTALL_PREFIX)
    QDir pluginsDir(QString("%1/lib/dfm").arg(INSTALL_PREFIX));
#else
    QDir pluginsDir("/usr/lib/dfm");
#endif
    qDebug() << "attempting to load thumbnailer plugins from" << pluginsDir.path();
    if (!pluginsDir.exists())
    {
        QDir appDir = applicationDirPath();
        appDir.cdUp();
        appDir.cd("plugins");
        foreach (const QFileInfo &file, appDir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot))
            if (file.isDir())
                loadPluginsFromDir(QDir(file.absoluteFilePath()));
        return;
    }
#else
    QDir pluginsDir(applicationDirPath());
    pluginsDir.cdUp();
    pluginsDir.cdUp();
#endif
    loadPluginsFromDir(pluginsDir);
}

void
Application::loadPluginsFromDir(const QDir &dir)
{
    foreach (const QString &fileName, dir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot))
    {
        QPluginLoader loader(dir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (ThumbInterface *it = qobject_cast<ThumbInterface *>(plugin))
        {
            qDebug() << "found thumbnailer:" << it->name() << it->description();
            if (DFM::Store::config.views.activeThumbIfaces.contains(it->name()))
                m_thumbIfaces.insert(it->name(), it);
            m_allThumbIfaces << it;
        }
        if (QFileInfo(dir.absoluteFilePath(fileName)).isDir())
            loadPluginsFromDir(QDir(dir.absoluteFilePath(fileName)));
    }
}

void
Application::activateThumbInterface(const QString &name)
{
    if (m_thumbIfaces.contains(name)) //already active
        return;

    if (!DFM::Store::config.views.activeThumbIfaces.contains(name))
        DFM::Store::config.views.activeThumbIfaces << name;
    for (int i = 0; i < m_allThumbIfaces.count(); ++i)
    {
        ThumbInterface *ti = m_allThumbIfaces.at(i);
        if (ti->name() == name)
        {
            activateThumbInterface(ti);
            return;
        }
    }
}

void
Application::deActivateThumbInterface(const QString &name)
{
    if (!m_thumbIfaces.contains(name)) //already inactive
        return;
    if (DFM::Store::config.views.activeThumbIfaces.contains(name))
        DFM::Store::config.views.activeThumbIfaces.removeOne(name);
    for (int i = 0; i < m_allThumbIfaces.count(); ++i)
    {
        ThumbInterface *ti = m_allThumbIfaces.at(i);
        if (ti->name() == name)
        {
            deActivateThumbInterface(ti);
            return;
        }
    }
}

QList<ThumbInterface *>
Application::thumbIfaces()
{
    return m_allThumbIfaces;
}

QList<ThumbInterface *>
Application::activeThumbIfaces()
{
    return m_thumbIfaces.values();
}

bool
Application::hasThumbIfaces()
{
    return !m_allThumbIfaces.isEmpty();
}

bool
Application::isActive(ThumbInterface *ti)
{
    return m_thumbIfaces.contains(ti->name());
}

void
Application::activateThumbInterface(ThumbInterface *ti)
{
    m_thumbIfaces.insert(ti->name(), ti);
}

void
Application::deActivateThumbInterface(ThumbInterface *ti)
 {
    m_thumbIfaces.remove(ti->name());
}

#if defined(HASX11)
#if 0

inline static void addState(Window win, Atom state)
{
    static Atom netWmState = XInternAtom(DPY, "_NET_WM_STATE", True);
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char *data = 0;
    bool exists = false;

    if (Success == XGetWindowProperty(DPY, win, netWmState, 0, (~0L), False, XA_ATOM, &type, &format, &nitems, &after, &data))
        if (nitems > 0)
        {
            for (int i = 0; i < (int)nitems; ++i)
                exists |= (bool)(((Atom *)data)[i] == state);
            XFree(data);
        }
    if (exists)
        return;

    XChangeProperty(DPY, win, netWmState, XA_ATOM, 32, PropModeAppend, (unsigned char *)&state, 1);

    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.message_type = netWmState;
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.display = DPY;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = state;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;
    if (!XSendEvent(DPY, QX11Info::appRootWindow(), False, SubstructureNotifyMask | SubstructureRedirectMask, &event))
        qDebug() << "failed to alter state of docks";
}

inline static void dockOps(Window win)
{
    static Atom states[2] = { XInternAtom(DPY, "_NET_WM_STATE_SKIP_TASKBAR", False), XInternAtom(DPY, "_NET_WM_STATE_SKIP_PAGER", False) };
    for (int i = 0; i < 2; ++i)
        addState(win, states[i]);
}

inline static void moveDockToDesktop(const Window &win, const Atom &atom, const int desktop)
{
    XChangeProperty(DPY, win, atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&desktop, 1);

    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = atom;
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.display = DPY;
    event.xclient.data.l[0] = desktop;
    event.xclient.data.l[1] = 0l;
    event.xclient.data.l[2] = 0l;
    event.xclient.data.l[3] = 0l;
    event.xclient.data.l[4] = 0l;
    if (!XSendEvent(DPY, win, False, PropertyChangeMask, &event))
        qDebug() << "failed to move dock";
}

#define ISDRAWER(_VAR_) _VAR_ && _VAR_->isFloating() && _VAR_->isWindow() && _VAR_->isVisible()
#define ISFLOATING(_VAR_) _VAR_ && _VAR_->isFloating() && _VAR_->isWindow()

bool
Application::x11EventFilter(XEvent *xe)
{
    if (!m_mainWindow)
        return QApplication::x11EventFilter(xe);

//    qDebug() << m_mainWindow->pos();
//    qDebug() << QCursor::pos();
//    qDebug() << xe->type;
//    static int inT = 0;
//    qDebug() << m_mainWindow->winId() << ++inT;
    switch (xe->type)
    {
    case MapNotify: //needed for unminimize
            foreach (DFM::Docks::DockWidget *dock, m_docks)
                if (ISFLOATING(dock))
                {
                    Window win = dock->winId();
                    dockOps(win);
                    if (xe->xmap.window == m_mainWindow->winId())
                        XMapWindow(DPY, win);
                }
        break;
    case UnmapNotify:  //needed for minimize
        if (xe->xmap.window == m_mainWindow->winId())
            foreach (DFM::Docks::DockWidget *dock, m_docks)
                if (ISFLOATING(dock))
                    XUnmapWindow(DPY, dock->winId());
        break;
    case PropertyNotify:
    {
        if (xe->xproperty.atom == netClientListStacking)
        {
            Atom type;
            int format;
            unsigned long nitems, after;
            unsigned char *data = 0;
            if (Success == XGetWindowProperty(DPY, QX11Info::appRootWindow(), netClientListStacking, 0, (~0L), False, XA_WINDOW, &type, &format, &nitems, &after, &data))
                if (data && (int)nitems > 1)
                {
                    QMap<Window, int> wMap;
                    for (int i = 0; i < (int)nitems; ++i)
                        wMap[((Window *)data)[i]] = i;

                    int floatCount = 0;
                    QList<DFM::Docks::DockWidget *> fd;
                    foreach(DFM::Docks::DockWidget *dock, m_docks)
                        if (ISDRAWER(dock))
                        {
                            ++floatCount;
                            fd << dock;
                        }
                    if (!floatCount)
                        return false;

                    if (floatCount > 1)
                    {
                        int lowestDock = (int)nitems+1, highestdock = 0;
                        for (int i = 0; i < floatCount; ++i)
                        {
                            lowestDock = qMin(lowestDock, wMap[fd[i]->winId()]);
                            highestdock = qMax(highestdock, wMap[fd[i]->winId()]);
                        }

                        if (wMap[m_mainWindow->winId()] < highestdock)
                            XRaiseWindow(DPY, m_mainWindow->winId());
                        else if (lowestDock+floatCount < wMap[m_mainWindow->winId()])
                        {
                            foreach(DFM::Docks::DockWidget *dock, fd)
                            {
                                dockOps(dock->winId());
                                XRaiseWindow(DPY, dock->winId());
                            }
                            XRaiseWindow(DPY, m_mainWindow->winId());
                        }
                    }
                    else if (floatCount)
                    {
                        if (wMap[m_mainWindow->winId()] < wMap[fd[0]->winId()])
                            XRaiseWindow(DPY, m_mainWindow->winId());
                        else if (wMap[fd[0]->winId()]+1 < wMap[m_mainWindow->winId()])
                        {
                            XRaiseWindow(DPY, fd[0]->winId());
                            dockOps(fd[0]->winId());
                            XRaiseWindow(DPY, m_mainWindow->winId());
                        }
                    }
                    XFree(data);
                    return false;
                }
        }
        if (xe->xproperty.atom == netWmDesktop && xe->xproperty.window == m_mainWindow->winId())
        {
            Atom type;
            int format;
            unsigned long nitems, after;
            unsigned char *data = 0;

            if (Success == XGetWindowProperty(DPY, m_mainWindow->winId(), netWmDesktop, 0, (~0L), False, XA_CARDINAL, &type, &format, &nitems, &after, &data))
                if (data && (int)nitems == 1)
                {
                    foreach(DFM::Docks::DockWidget *dock, m_docks)
                        if(ISDRAWER(dock))
                            moveDockToDesktop(dock->winId(), netWmDesktop, ((int *)data)[0]);
                    XFree(data);
                    return false;
                }
        }
        break;
    }
    default: break;
    }
    return QApplication::x11EventFilter(xe);
}
#endif
#endif
