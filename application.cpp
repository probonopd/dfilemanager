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

#ifdef Q_WS_X11
Atom Application::m_netWmDesktop = 0;
Atom Application::m_netClientListStacking = 0;

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

inline static void moveDockToDesktop(const Window &win, const Atom &atom, const int &desktop)
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
        return false;

//    qDebug() << widgetAt(QCursor::pos());

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
        if (xe->xproperty.atom == m_netClientListStacking)
        {
            Atom type;
            int format;
            unsigned long nitems, after;
            unsigned char *data = 0;
            if (Success == XGetWindowProperty(DPY, QX11Info::appRootWindow(), m_netClientListStacking, 0, (~0L), False, XA_WINDOW, &type, &format, &nitems, &after, &data))
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
                        int lowestDock = (int)nitems+1; //just a really high number....
                        int highestdock = 0;
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
        if (xe->xproperty.atom == m_netWmDesktop && xe->xproperty.window == m_mainWindow->winId())
        {
            Atom type;
            int format;
            unsigned long nitems, after;
            unsigned char *data = 0;

            if (Success == XGetWindowProperty(DPY, m_mainWindow->winId(), m_netWmDesktop, 0, (~0L), False, XA_CARDINAL, &type, &format, &nitems, &after, &data))
                if (data && (int)nitems == 1)
                {
                    foreach(DFM::Docks::DockWidget *dock, m_docks)
                        if(ISDRAWER(dock))
                            moveDockToDesktop(dock->winId(), m_netWmDesktop, ((int *)data)[0]);
                    XFree(data);
                    return false;
                }
        }
        break;
    }
    default: break;
    }
    return false;
}
#endif
