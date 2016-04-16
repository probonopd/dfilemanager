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

#include <QImageReader>
#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QSettings>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QDebug>

#include "fsmodel.h"
#include "detailsview.h"
#include "flow.h"
#include "flowview.h"
#include "mainwindow.h"
#include "config.h"
#include "infowidget.h"
#include "operations.h"

using namespace DFM;

FlowView::FlowView(QWidget *parent)
    : QAbstractItemView(parent)
    , m_splitter(0)
    , m_dView(0)
    , m_flow(0)
{
    m_dView = new DetailsView(this);
    m_dView->setRootIsDecorated(false);
    m_dView->setItemsExpandable(false);

    m_flow = new Flow(this);

    m_splitter = new QSplitter(this);
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(m_flow);
    m_splitter->addWidget(m_dView);
    m_splitter->restoreState(Store::config.views.flowSize);
    m_splitter->setFrameStyle(0);
    m_splitter->setContentsMargins(0,0,0,0);
//    m_splitter->setHandleWidth(style()->pixelMetric(QStyle::PM_SplitterWidth));

    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);

    connect(m_flow, SIGNAL(centerIndexChanged(QModelIndex)), this, SLOT(flowCurrentIndexChanged(QModelIndex)));
    connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(saveSplitter()));
    connect(m_dView, SIGNAL(entered(QModelIndex)), this, SIGNAL(entered(QModelIndex)));
    connect(m_dView, SIGNAL(opened(QModelIndex)), this, SIGNAL(opened(QModelIndex)));
    connect(m_dView, SIGNAL(viewportEntered()), this, SIGNAL(viewportEntered()));
    connect(m_dView, SIGNAL(newTabRequest(QModelIndex)), this, SIGNAL(newTabRequest(QModelIndex)));
}

FlowView::~FlowView(){}

void
FlowView::saveSplitter()
{
    Store::config.views.flowSize = m_splitter->saveState();
}

void
FlowView::setModel(QAbstractItemModel *model)
{
    m_dView->setModel(model);
    m_flow->setModel(static_cast<FS::Model *>(model));
    QAbstractItemView::setModel(model);
}

void
FlowView::setRootIndex(const QModelIndex &rootIndex)
{
    m_dView->setRootIndex(rootIndex);
    m_flow->setRootIndex(rootIndex);
    QAbstractItemView::setRootIndex(rootIndex);
}

void
FlowView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_dView->setSelectionModel(selectionModel);
    m_flow->setSelectionModel(selectionModel);
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,SLOT(treeCurrentIndexChanged(QItemSelection,QItemSelection)));
    QAbstractItemView::setSelectionModel(selectionModel);
}

void
FlowView::treeCurrentIndexChanged(QItemSelection,QItemSelection)
{
    const QModelIndex &index = m_dView->selectionModel()->currentIndex();
    if (index.isValid())
    {
        m_flow->animateCenterIndex(index);
        m_dView->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void
FlowView::resizeEvent(QResizeEvent *e)
{
    m_splitter->resize(e->size());
    e->accept();
}

bool
FlowView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    return m_dView->edit(index, trigger, event);
}

void
FlowView::scrollToTop()
{
    m_dView->scrollToTop();
    m_flow->setCenterIndex(rootIndex().child(0, 0));
}

void
FlowView::scrollToBottom()
{
    m_dView->scrollToBottom();
    m_flow->setCenterIndex(rootIndex().child(model()->rowCount(rootIndex())-1, 0));
}

void FlowView::addActions(QList<QAction *> actions) { m_dView->addActions(actions); }
void FlowView::flowCurrentIndexChanged(const QModelIndex &index) { m_dView->scrollTo(index, QAbstractItemView::EnsureVisible); }

/*
 * pure virtuals
 */

QModelIndex
FlowView::indexAt(const QPoint &point) const
{
    if (m_dView->geometry().contains(point))
        return m_dView->indexAt(point+m_dView->frameGeometry().topLeft());
    return QModelIndex();
}

void
FlowView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    m_flow->setCenterIndex(index);
    m_dView->scrollTo(index, hint);
}

QRect
FlowView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();

    QRect r = m_dView->visualRect(index);
    if (r.isValid())
    {
        r.translate(m_dView->frameGeometry().topLeft());
        return r;
    }
    return QRect();
}

int
FlowView::horizontalOffset() const
{
    return 0;
}

int
FlowView::verticalOffset() const
{
    return 0;
}

bool
FlowView::isIndexHidden(const QModelIndex &index) const
{
    return m_dView->isIndexHidden(index);
}

void
FlowView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags)
{
    Q_UNUSED(rect);
    Q_UNUSED(flags);
}

QModelIndex
FlowView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    return m_dView->moveCursor(cursorAction, modifiers);
}

QRegion
FlowView::visualRegionForSelection(const QItemSelection &selection) const
{
    return QRegion();
}
