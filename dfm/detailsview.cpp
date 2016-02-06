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

#include <QMenu>
#include <QDragMoveEvent>
#include <QDebug>
#include <QHeaderView>
#include <qmath.h>
#include <QTextEdit>
#include <QMessageBox>
#include <QPainter>
#include "viewcontainer.h"
#include "detailsview.h"
#include "mainwindow.h"
#include "config.h"
#include "objects.h"

using namespace DFM;

class DetailsDelegate : public FileItemDelegate
{
public:
    DetailsDelegate(QObject *parent) : FileItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (!index.isValid())
            return;
        painter->save();
        const bool isHovered = option.state & QStyle::State_MouseOver;
        const bool isSelected = option.state & QStyle::State_Selected;
        if (index.column() > 0 &&  !isHovered && !isSelected)
            painter->setOpacity(0.66);
        QStyledItemDelegate::paint(painter, option, index);
        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return FileItemDelegate::sizeHint(option, index) + QSize(0, Store::config.views.detailsView.rowPadding*2);
    }
};

DetailsView::DetailsView(QWidget *parent)
    : QTreeView(parent)
    , m_pressPos(QPoint())
    , m_pressedIndex(0)
{
    ScrollAnimator::manage(this);
    setItemDelegate(new DetailsDelegate(this));
    header()->setStretchLastSection(false);
    setUniformRowHeights(true);
    setSelectionMode(ExtendedSelection);
    setSortingEnabled(true);
    setEditTriggers(SelectedClicked | EditKeyPressed);
    setExpandsOnDoubleClick(false);
    setDragDropMode(DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setVerticalScrollMode(ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlternatingRowColors(Store::config.views.detailsView.altRows);
    header()->setSortIndicator(0, Qt::AscendingOrder);

    connect(this, SIGNAL(sortIndicatorChanged(int,int)), container(), SIGNAL(sortingChanged(int,int)));
    connect(header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortingChanged(int,Qt::SortOrder)));
    connect(container(), SIGNAL(sortingChanged(int,int)), this, SLOT(sortingChanged(int,int)));
    connect(container(), SIGNAL(settingsChanged()), this, SLOT(readSettings()));
}

DetailsView::~DetailsView()
{

}

void
DetailsView::readSettings()
{
    setAlternatingRowColors(Store::config.views.detailsView.altRows);
}

ViewContainer
*DetailsView::container()
{
    QWidget *w = this;
    while (w)
    {
        if (ViewContainer *c = qobject_cast<ViewContainer *>(w))
            return c;
        w = w->parentWidget();
    }
    return 0;
}

void
DetailsView::sortingChanged(const int column, const int order)
{
    if (header() && model() && column>-1&&column<header()->count())
    {
        header()->blockSignals(true);
        header()->setSortIndicator(column, (Qt::SortOrder)order);
        header()->blockSignals(false);
    }
}

void
DetailsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTreeView::mouseDoubleClickEvent(event);
    const QModelIndex &index = indexAt(event->pos());
    if (index.isValid()
            && !Store::config.views.singleClick
            && !event->modifiers()
            && event->button() == Qt::LeftButton
            && state() == NoState)
        emit opened(index);
}

void
DetailsView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    for (int i = 0; i<header()->count(); ++i)
#if QT_VERSION < 0x050000
        header()->setResizeMode(i, QHeaderView::Fixed);
#else
        header()->setSectionResizeMode(i, QHeaderView::Fixed);
#endif
}

void
DetailsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        clearSelection();
    if (event->key() == Qt::Key_Return
         && event->modifiers() == Qt::NoModifier
         && state() == NoState)
    {
        if (selectionModel()->selectedIndexes().count())
            foreach (const QModelIndex &index, selectionModel()->selectedIndexes())
                if (!index.column())
                    emit opened(index);
        event->accept();
        return;
    }
    DViewBase::keyPressEvent(event);
    QTreeView::keyPressEvent(event);
}

void
DetailsView::contextMenuEvent(QContextMenuEvent *event)
{
    const QString &file = indexAt(event->pos()).data(FS::FilePathRole).toString();
    if (MainWindow *mw = MainWindow::currentWindow())
        mw->rightClick(file, event->globalPos());
}

void
DetailsView::mouseReleaseEvent(QMouseEvent *e)
{
    setDragEnabled(true);
    const QModelIndex &index = indexAt(e->pos());

    if (!index.isValid())
    {
        QTreeView::mouseReleaseEvent(e);
        return;
    }

    if (Store::config.views.singleClick
            && !e->modifiers()
            && e->button() == Qt::LeftButton
            && m_pressedIndex == index.internalPointer()
            && visualRect(index).contains(e->pos()) //clicking on expanders should not open the dir...
            && !state()
            && !index.column())
    {
        emit opened(index);
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
    if (event->modifiers() & Qt::MetaModifier || indexAt(event->pos()).column())
        setDragEnabled(false);
    m_pressedIndex = indexAt(event->pos()).internalPointer();
    m_pressPos = event->pos();
    QTreeView::mousePressEvent(event);
}

void
DetailsView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::CTRL)
    {
        MainWindow *mw = MainWindow::window(this);
        if (QSlider *s = mw->iconSizeSlider())
            s->setValue(s->value()+(e->delta()>0?1:-1));
    }
    else
        QTreeView::wheelEvent(e);
    e->accept();
}

void
DetailsView::resizeEvent(QResizeEvent *event)
{
    const int w = event->size().width();
    setColumnWidth(0, qRound(w*0.6f));
    setColumnWidth(1, qRound(w*0.12f));
    setColumnWidth(2, qRound(w*0.08f));
    setColumnWidth(3, qRound(w*0.2f));
//    setColumnWidth(4, qRound(w*0.07f));
//    for (int i = 1; i < header()->count(); ++i)
//        setColumnWidth(i, w*0.1f);
}

bool
DetailsView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    if (!index.isValid())
        return false;
    QModelIndex idx = index;
    if (idx.column())
        idx = idx.sibling(idx.row(), 0);
    return QTreeView::edit(idx, trigger, event);
}
