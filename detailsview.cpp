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

using namespace DFM;

class DetailsDelegate : public QStyledItemDelegate
{
protected:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QStyledItemDelegate::sizeHint(option, index) + QSize(0, Configuration::config.views.detailsView.rowPadding*2);
    }
};

DetailsView::DetailsView(QWidget *parent) :
    QTreeView(parent)
{
    setItemDelegate(new DetailsDelegate());
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    setRootIsDecorated(false);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setExpandsOnDoubleClick(false);
    setItemsExpandable(false);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void
DetailsView::resizeEvent(QResizeEvent *event)
{
    setColumnWidth(0,viewport()->width()-320);
    setColumnWidth(3,120);
    QTreeView::resizeEvent(event);
}

void
DetailsView::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Return && event->modifiers() == Qt::NoModifier && state() != QAbstractItemView::EditingState )
    {
        if ( selectionModel()->selectedRows().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedRows() )
                emit activated(index);
        else if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                emit activated(index);
    }
    QTreeView::keyPressEvent(event);
}

void
DetailsView::setFilter(QString filter)
{
    for(int i = 0; i < model()->rowCount(rootIndex()); i++)
    {
        if(model()->index(i,0,rootIndex()).data().toString().toLower().contains(filter.toLower()))
            setRowHidden(i,rootIndex(), false);
        else
            setRowHidden(i,rootIndex(),true);
    }
}

void
DetailsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu popupMenu;
    if ( Configuration::customActions().count() )
        popupMenu.addMenu(Configuration::customActionsMenu());
    popupMenu.addActions( actions() );
    const QString &file = static_cast<FileSystemModel *>( model() )->filePath( indexAt( event->pos() ) );
    QMenu openWith( tr( "Open With" ), this );
    openWith.addActions( Configuration::openWithActions( file ) );
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
    if (e->button() == Qt::MiddleButton)
        if (indexAt(e->pos()).isValid())
            emit newTabRequest(indexAt(e->pos()));
    setDragEnabled(true);
    QTreeView::mouseReleaseEvent(e);
}

void
DetailsView::mousePressEvent(QMouseEvent *event)
{
    if(event->modifiers() == Qt::MetaModifier)
        setDragEnabled(false);
    QTreeView::mousePressEvent(event);
}


