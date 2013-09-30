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
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
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
    , m_userPlayed(false)
{
    setItemDelegate(new DetailsDelegate());
    setUniformRowHeights(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
//    setRootIsDecorated(false);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setExpandsOnDoubleClick(false);
//    setItemsExpandable(false);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void
DetailsView::dirLoaded(const QString &path)
{
    if ( path != m_model->rootPath()
         || path != m_model->filePath(rootIndex())
         || !isVisible()
         || m_detailsWidth > 0 )
        return;

    int w = width()-(verticalScrollBar()->isVisible()*style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    setColumnWidth(0, w);

    for ( int i = 1; i < m_model->columnCount(rootIndex()); ++i )
    {
        resizeColumnToContents(i);
        m_detailsWidth+=columnWidth(i);
    }

    w-=m_detailsWidth;
    if ( m_detailsWidth < 1 || w > width()-16 )
        qDebug() << "something went wrong when resizing columns in detailsview" << m_detailsWidth;
    setColumnWidth(0, w);
}

void
DetailsView::sortingChanged(const int column, const Qt::SortOrder order)
{
    if ( header() && model() == m_model && column>-1&&column<header()->count() )
    {
        header()->blockSignals(true);
        header()->setSortIndicator(column, order);
        header()->blockSignals(false);
    }
}

void
DetailsView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    m_model = qobject_cast<FileSystemModel *>(model);
    connect(m_model, SIGNAL(directoryLoaded(QString)), this, SLOT(dirLoaded(QString)));
//    connect(m_proxyModel, SIGNAL(sortingChanged(int,Qt::SortOrder)), this, SLOT(sortingChanged(int,Qt::SortOrder)));
}

void
DetailsView::resizeEvent(QResizeEvent *event)
{
    QTreeView::resizeEvent(event);
    if ( !m_userPlayed )
        setColumnWidth(0, viewport()->width()-m_detailsWidth);
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
    if (e->button() == Qt::MiddleButton)
        if (indexAt(e->pos()).isValid())
        {
            e->accept();
            emit newTabRequest(indexAt(e->pos()));
            return;
        }
    setDragEnabled(true);
    QTreeView::mouseReleaseEvent(e);
}

void
DetailsView::mousePressEvent(QMouseEvent *event)
{
    if (event->modifiers() == Qt::MetaModifier)
        setDragEnabled(false);
    QTreeView::mousePressEvent(event);
}

void
DetailsView::showEvent(QShowEvent *e)
{
    QTreeView::showEvent(e);
    dirLoaded(m_model->filePath(rootIndex()));
}


