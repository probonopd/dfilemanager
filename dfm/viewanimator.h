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
namespace DFM
{
class ViewAnimator : public QObject
{
    Q_OBJECT
public:
    static ViewAnimator *manage(QAbstractItemView *view);
    static int hoverLevel(QAbstractItemView *view, const QModelIndex &index);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    explicit ViewAnimator(QObject *parent = 0);
    const int hoverLevelForIndex(const QModelIndex &index) const;

public slots:
    void indexHovered(const QModelIndex &index);
    void removeHoveredIndex();
    void animEvent();
    void rowsRemoved(const QModelIndex & parent, int start, int end);
    inline void clear() { m_vals.clear(); }
    void removeView(QObject *view);

private:
    static QMap<QAbstractItemView *, ViewAnimator *> s_views;
    QMap<QModelIndex, int> m_vals;
    QTimer *m_timer;
    QAbstractItemView *m_view;
    QModelIndex m_current;
};
} //end namespace

#endif // VIEWANIMATOR_H
