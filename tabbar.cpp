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


#include "tabbar.h"

using namespace DFM;

TabBar::TabBar(QWidget *parent) : QTabBar(parent)
{
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setDocumentMode(true);
    setTabsClosable(true);
    setMovable(true);
    setDrawBase(true);
    setExpanding(false);
    setElideMode(Qt::ElideRight);
}

void
TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()))
    {
        emit newTabRequest();
        event->accept();
        return;
    }
    QTabBar::mouseDoubleClickEvent(event);
}

void
TabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && tabAt(event->pos()) > -1)
    {
        emit tabCloseRequested(tabAt(event->pos()));
        event->accept();
        return;
    }
    QTabBar::mouseReleaseEvent(event);
}
