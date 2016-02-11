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


#include "viewanimator.h"
#include "fsmodel.h"
#include <QTreeView>
#include <QScrollBar>
#include "globals.h"

using namespace DFM;

QMap<QAbstractItemView *, ViewAnimator *> ViewAnimator::s_views;

ViewAnimator::ViewAnimator(QObject *parent) : QObject(parent),
    m_timer(new QTimer(this)),
    m_view(static_cast<QAbstractItemView *>(parent))
{
    //    m_animTimer->setInterval(20);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(animEvent()));
    connect(m_view, SIGNAL(entered(QModelIndex)), this, SLOT(indexHovered(QModelIndex)));
    connect(m_view, SIGNAL(viewportEntered()), this, SLOT(removeHoveredIndex()));
    connect(m_view->model(), SIGNAL(layoutAboutToBeChanged()), this, SLOT(clear()));
    connect(m_view, SIGNAL(destroyed(QObject*)), this, SLOT(removeView(QObject*)));
    connect(m_view->model(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(rowsRemoved(QModelIndex,int,int)));
    //    connect((static_cast<DFM::FileSystemModel *>(m_view->model())), SIGNAL(rootPathChanged(QString)), this, SLOT(clear()));
    m_view->setMouseTracking(true);
    m_view->installEventFilter(this);
}

void
ViewAnimator::indexHovered(const QModelIndex &index)
{
    if (index.isValid() && index != m_current && index.flags() & Qt::ItemIsEnabled)
    {
        m_current = index;
        if (!m_vals.contains(m_current))
            m_vals.insert(m_current, 0);
        if (!m_timer->isActive())
            m_timer->start(40);
    }
}

void
ViewAnimator::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i<=end; ++i)
        m_vals.remove(m_view->model()->index(i, 0, parent));
}

void
ViewAnimator::removeHoveredIndex()
{
    m_current = QModelIndex();
    if (!m_timer->isActive())
        m_timer->start(40);
}

void
ViewAnimator::animEvent()
{
    bool needRunning(false);
    QMapIterator<QModelIndex, int> it(m_vals);
    while (it.hasNext())
    {
        QModelIndex index(it.next().key());
        const bool mouse(index == m_current);
        const int val(it.value());
        if (mouse && val < Steps)
        {
            needRunning = true;
            m_vals.insert(index, (val&~1)+2);
        }
        else if (!mouse && val > 0)
        {
            needRunning = true;
            m_vals.insert(index, val-1);
        }
        else if (!mouse && val == 0)
            m_vals.remove(index);

        m_view->update(index);
    }
    if (!needRunning)
        m_timer->stop();
}

bool
ViewAnimator::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_view && ev->type() == QEvent::Leave)
        removeHoveredIndex();
    return false;
}

ViewAnimator
*ViewAnimator::manage(QAbstractItemView *view)
{
    if (s_views.contains(view))
        return s_views.value(view);

    ViewAnimator *va(new ViewAnimator(view));
    s_views.insert(view, va);
    return va;
}

const int
ViewAnimator::hoverLevelForIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return m_vals.value(index, 0);
    return 0;
}

int
ViewAnimator::hoverLevel(QAbstractItemView *view, const QModelIndex &index)
{
    if (index.isValid() && s_views.contains(view))
        return s_views.value(view)->hoverLevelForIndex(index);
    return 0;
}

void
ViewAnimator::removeView(QObject *view)
{
    QAbstractItemView *v(static_cast<QAbstractItemView *>(view));
    if (s_views.contains(v))
        s_views.remove(v);
}
