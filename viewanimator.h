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


#ifndef VIEWANIMATOR_H
#define VIEWANIMATOR_H

#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QTimer>
#include <QEvent>

class ViewAnimator : public QObject
{
    Q_OBJECT
public:
    static void manage(QAbstractItemView *view);
    static inline int hoverLevel(QAbstractItemView *view, const QModelIndex &index) { return view->findChild<ViewAnimator*>()->hoverLevelForIndex(index); }

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    explicit ViewAnimator(QObject *parent = 0);
    inline int hoverLevelForIndex(const QModelIndex &index) const { return m_hoverLevel[index]; }

public slots:
    void indexHovered(const QModelIndex &index);
    void removeHoveredIndex();
    void animEvent();
    void rowsRemoved( const QModelIndex & parent, int start, int end );
    inline void clear() { m_hoverLevel.clear(); }

private:
    QMap<QModelIndex, int> m_hoverLevel;
    const QAbstractItemModel *m_model;
    QModelIndex m_hoveredIndex;
    QTimer *m_animTimer;
    QAbstractItemView *m_view;
    int m_totalIndex;

};

#endif // VIEWANIMATOR_H
