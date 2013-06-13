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


#ifndef DETAILSVIEW_H
#define DETAILSVIEW_H

#include <QTreeView>
#include <QMenu>
#include <QDragMoveEvent>
#include <QDebug>
#include <QHeaderView>

namespace DFM
{

class DetailsView : public QTreeView
{
    Q_OBJECT
public:
    explicit DetailsView(QWidget *parent = 0);
    void resizeEvent(QResizeEvent *event);
    void setFilter(QString filter);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *);

signals:
    void newTabRequest(QModelIndex path);
};

}

#endif // DETAILSVIEW_H
