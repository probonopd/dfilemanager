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


#include "detailsview.h"
#include "viewcontainer.h"
#include "filesystemmodel.h"
#include "mainwindow.h"
#include <qmath.h>

using namespace DFM;

class DetailsDelegate : public QStyledItemDelegate
{
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if ( !index.data().isValid() )
            return;
        painter->save();
        const bool isHovered = option.state & QStyle::State_MouseOver;
        const bool isSelected = option.state & QStyle::State_Selected;
        if ( index.column() > 0 &&  !isHovered && !isSelected )
            painter->setOpacity(0.66);
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QStyledItemDelegate::sizeHint(option, index) + QSize(0, Store::config.views.detailsView.rowPadding*2);
    }
};

DetailsView::DetailsView(QWidget *parent)
    : QTreeView(parent)
    , m_model(0)
    , m_pressPos(QPoint())
    , m_pressedIndex(QModelIndex())
{
    setItemDelegate(new DetailsDelegate());
    header()->setStretchLastSection(false);
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setExpandsOnDoubleClick(false);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, SIGNAL(sortIndicatorChanged(int,int)), container(), SIGNAL(sortingChanged(int,int)));
    connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortingChanged(int,Qt::SortOrder)));
    connect(container(), SIGNAL(sortingChanged(int,int)), this, SLOT(sortingChanged(int,int)));
}

ViewContainer
*DetailsView::container()
{
    QWidget *w = this;
    while ( w )
    {
        if ( ViewContainer *c = qobject_cast<ViewContainer *>(w) )
            return c;
        w = w->parentWidget();
    }
    return 0;
}

void
DetailsView::sortingChanged(const int column, const int order)
{
    if ( header() && model() == m_model && column>-1&&column<header()->count() )
    {
        header()->blockSignals(true);
        header()->setSortIndicator(column, (Qt::SortOrder)order);
        header()->blockSignals(false);
    }
}

void
DetailsView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    if ( !index.data().isValid() )
        return;
    QTreeView::drawBranches(painter, rect, index);
}

void
DetailsView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    m_model = qobject_cast<FileSystemModel *>(model);
    for ( int i = 0; i<header()->count(); ++i )
        header()->setResizeMode(i, QHeaderView::Fixed);
}

void
DetailsView::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Escape )
        clearSelection();
    if ( event->key() == Qt::Key_Return
         && event->modifiers() == Qt::NoModifier
         && state() == NoState )
    {
        if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                if ( !index.column() )
                    emit activated(index);
        event->accept();
        return;
    }
    QTreeView::keyPressEvent(event);
}

void
DetailsView::setFilter(QString filter)
{
    for (int i = 0; i < model()->rowCount(rootIndex()); i++)
    {
        if (model()->index(i,0,rootIndex()).data().toString().toLower().contains(filter.toLower()))
            setRowHidden(i,rootIndex(), false);
        else
            setRowHidden(i,rootIndex(),true);
    }
}

void
DetailsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu popupMenu;
    if ( Store::customActions().count() )
        popupMenu.addMenu(Store::customActionsMenu());
    popupMenu.addActions( actions() );
    const QString &file = static_cast<FileSystemModel *>( model() )->filePath( indexAt( event->pos() ) );
    QMenu openWith( tr( "Open With" ), this );
    openWith.addActions( Store::openWithActions( file ) );
    foreach( QAction *action, actions() )
    {
        popupMenu.addAction( action );
        if ( action->objectName() == "actionDelete" )
            popupMenu.insertSeparator( action );
        if ( action->objectName() == "actionCustomCmd" )
            popupMenu.insertMenu( action, &openWith );
    }
    popupMenu.exec( event->globalPos() );
}

void
DetailsView::mouseReleaseEvent(QMouseEvent *e)
{
    setDragEnabled(true);
    const QModelIndex &index = indexAt(e->pos());

    if ( !index.isValid() )
    {
        QTreeView::mouseReleaseEvent(e);
        return;
    }

    if ( Store::config.views.singleClick
         && !e->modifiers()
         && e->button() == Qt::LeftButton
         && m_pressedIndex == index
         && visualRect(index).contains(e->pos())
         && !state() )
    {
        emit activated(index);
        e->accept();
        return;
    }

    if (e->button() == Qt::MiddleButton
            && m_pressPos == e->pos()
            && !e->modifiers())
    {
        e->accept();
        emit newTabRequest(index);
        return;
    }

    QTreeView::mouseReleaseEvent(e);
}

void
DetailsView::mousePressEvent(QMouseEvent *event)
{
    if (event->modifiers() == Qt::MetaModifier)
        setDragEnabled(false);
    m_pressedIndex = indexAt(event->pos());
    m_pressPos = event->pos();
    QTreeView::mousePressEvent(event);
}

void
DetailsView::resizeEvent(QResizeEvent *event)
{
    const int w = event->size().width();
    setColumnWidth(0, qRound(w*0.6f));
    setColumnWidth(1, qRound(w*0.12f));
    setColumnWidth(2, qRound(w*0.08f));
    setColumnWidth(3, qRound(w*0.2f));
//    for ( int i = 1; i < header()->count(); ++i )
//        setColumnWidth(i, w*0.1f);
}
