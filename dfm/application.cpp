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
#include "mainwindow.h"
#include "config.h"
#include <QSharedMemory>
#include <QDebug>
#include <QDesktopServices>
#include <QPluginLoader>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

#ifdef Q_WS_X11
#if 0
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/X.h>
static Atom netWmDesktop = 0;
static Atom netClientListStacking = 0;
#endif
#endif

Application::Application(int &argc, char *argv[], const QString &key)
    : QApplication(argc, argv)
    , m_sharedMem(new QSharedMemory(key, this))
    , m_key(key)
    , m_fsWatcher(0)
{
#if QT_VERSION < 0x050000
    const QString &dirPath(QDesktopServices::storageLocation(QDesktopServices::TempLocation));
#else
    const QString &dirPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
#endif
    if ( !QFileInfo(dirPath).exists() )
        QDir::root().mkpath(dirPath);

    m_filePath = QDir(dirPath).absoluteFilePath("dfm_message_file");
    m_file.setFileName(m_filePath);
    if ( m_sharedMem->attach() )
        m_isRunning = true;
    else
    {
        setOrganizationName("dfm");
        m_isRunning = false;
        if ( !m_sharedMem->create(1) )
            qDebug() << "failed to create shared memory";
        //        connect(m_localServer, SIGNAL(newConnection()), this, SLOT(receiveMessage()));
        //        m_localServer->listen(key);

        if ( m_file.open(QFile::WriteOnly|QFile::Truncate) )
        {
            m_file.write(QByteArray());
            m_file.close();
        }
        m_fsWatcher = new QFileSystemWatcher(this);
        m_fsWatcher->addPath(m_filePath);

        connect(m_fsWatcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
        DFM::Store::readConfig();
    }
#ifdef Q_WS_X11
#if 0
    netWmDesktop = XInternAtom(DPY, "_NET_WM_DESKTOP", False);
    netClientListStacking = XInternAtom(DPY, "_NET_CLIENT_LIST_STACKING", False);
#endif
#endif
}

static QDateTime s_lastModified;

void
Application::fileChanged(const QString &file)
{
    if ( isRunning() || (s_lastModified.isValid()&&s_lastModified==QFileInfo(m_file).lastModified()) )
        return;

    s_lastModified = QFileInfo(m_file).lastModified();

    if (m_file.open(QFile::ReadOnly))
    {
        QTextStream out(&m_file);
        QStringList message;
        while (!out.atEnd())
            message << out.readLine();
        emit lastMessage(message);
        m_file.close();
    }
}

bool
Application::setMessage(const QStringList &message)
{
    if ( !isRunning() )
        return false;

    if (m_file.open(QFile::WriteOnly|QFile::Truncate))
    {
        QTextStream out(&m_file);
        foreach (const QString &string, message)
            out << QString("%1\n").arg(string);
        m_file.close();
    }
}


void
Application::loadPlugins()
{
#ifdef Q_OS_UNIX
    QDir pluginsDir("/usr/lib/dfm");
    if ( !pluginsDir.exists() )
    {
        QDir appDir = applicationDirPath();
        appDir.cdUp();
        appDir.cd("plugins");
        foreach ( const QFileInfo &file, appDir.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot) )
            if ( file.isDir() )
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
Application::loadPluginsFromDir(const QDir dir)
{
    foreach (QString fileName, dir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot))
    {
        QPluginLoader loader(dir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if ( ThumbInterface *it = qobject_cast<ThumbInterface *>(plugin) )
        {
            if ( DFM::Store::config.views.activeThumbIfaces.contains(it->name()) )
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
    if ( m_thumbIfaces.contains(name) ) //already active
        return;

    if ( !DFM::Store::config.views.activeThumbIfaces.contains(name) )
        DFM::Store::config.views.activeThumbIfaces << name;
    for ( int i = 0; i < m_allThumbIfaces.count(); ++i )
    {
        ThumbInterface *ti = m_allThumbIfaces.at(i);
        if ( ti->name() == name )
        {
            activateThumbInterface(ti);
            return;
        }
    }
}

void
Application::deActivateThumbInterface(const QString &name)
{
    if ( !m_thumbIfaces.contains(name) ) //already inactive
        return;
    if ( DFM::Store::config.views.activeThumbIfaces.contains(name) )
        DFM::Store::config.views.activeThumbIfaces.removeOne(name);
    for ( int i = 0; i < m_allThumbIfaces.count(); ++i )
    {
        ThumbInterface *ti = m_allThumbIfaces.at(i);
        if ( ti->name() == name )
        {
            deActivateThumbInterface(ti);
            return;
        }
    }
}

#ifdef Q_WS_X11
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
        addState( win, states[i] );
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
