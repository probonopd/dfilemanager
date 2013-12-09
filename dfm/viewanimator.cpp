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
#include "filesystemmodel.h"
#include <QTreeView>

ViewAnimator::ViewAnimator(QObject *parent) : QObject(parent),
    m_totalIndex(0),
    m_animTimer(new QTimer(this)),
    m_view(static_cast<QAbstractItemView *>(parent)),
    m_model(0)
{
    connect(m_animTimer, SIGNAL(timeout()), this, SLOT(animEvent()));
    connect(m_view, SIGNAL(entered(QModelIndex)), this, SLOT(indexHovered(QModelIndex)));
    connect(m_view, SIGNAL(viewportEntered()), this, SLOT(removeHoveredIndex()));
//    connect((static_cast<DFM::FileSystemModel *>(m_view->model())), SIGNAL(rootPathChanged(QString)), this, SLOT(clear()));
    m_view->installEventFilter(this);
}

void
ViewAnimator::indexHovered(const QModelIndex &index)
{
    if ( !m_model && index.isValid() )
    {
        m_model = index.model();
        connect(m_model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsRemoved(QModelIndex,int,int)));
    }
    if ( index.isValid() && index.flags() & Qt::ItemIsEnabled )
    {
        m_hoveredIndex = index;
        if (!m_animTimer->isActive())
            m_animTimer->start(30);
    }
}

void
ViewAnimator::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    for ( int i = start; i<=end; ++i )
    {
        const QModelIndex &index = m_model->index(i, 0, parent);
        m_hoverLevel.remove(index);
    }
}

void
ViewAnimator::removeHoveredIndex()
{
    m_hoveredIndex = QModelIndex();
    if ( m_totalIndex && !m_animTimer->isActive() )
        m_animTimer->start( 50 );
}

void
ViewAnimator::animEvent()
{
    if ( m_hoveredIndex != QModelIndex() )
        if ( m_hoverLevel[m_hoveredIndex] == 8 )  //we have decided to hover one item longer then 400 ms... no reason to keep timer running.
        {
            m_animTimer->stop();
            return;
        }
    m_totalIndex = 0;
    if ( m_hoverLevel[m_hoveredIndex] < 8 )
        m_hoverLevel[m_hoveredIndex]++;

    if ( m_hoveredIndex != QModelIndex() )
        m_totalIndex = m_hoverLevel[m_hoveredIndex];

    foreach ( const QModelIndex &index, m_hoverLevel.keys() )
    {
        bool exists = false;
        if ( const DFM::FileSystemModel *fsModel = qobject_cast<const DFM::FileSystemModel *>(m_view->model()) )
        {
            if ( index.isValid() && fsModel->index( index.row(), 0, m_view->rootIndex()) == index )
                exists = true;
        }
        else
            exists = index.isValid();
        if ( exists )
        if ( m_hoverLevel[index] > 0 )
        {
            if ( index != m_hoveredIndex )
            {
                m_hoverLevel[index]--;
                m_totalIndex += m_hoverLevel[index];
            }
            m_view->update( index );
//            if ( QTreeView *tv = qobject_cast<QTreeView *>(m_view) )
//                if ( DFM::FileSystemModel *fsm = qobject_cast<DFM::FileSystemModel *>(tv->model()) )
//                    for ( int i = 0; i < tv->model()->columnCount(index.parent()); ++i )
//                        tv->update( fsm->index(index.row(), i) );
        }
    }
    if ( !m_totalIndex )
        m_animTimer->stop();
}

bool
ViewAnimator::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_view && ev->type() == QEvent::Leave)
        removeHoveredIndex();
    return false;
}

void
ViewAnimator::manage(QAbstractItemView *view)
{
    if ( view->findChild<ViewAnimator*>() )
        return;
    new ViewAnimator(view);
}
