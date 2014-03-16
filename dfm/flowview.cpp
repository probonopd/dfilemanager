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


#include "flowview.h"
#include "mainwindow.h"
#include "config.h"
#include "infowidget.h"
#include "operations.h"
#include <QImageReader>

using namespace DFM;

FlowView::FlowView(QWidget *parent)
    : QWidget(parent)
    , m_splitter(new QSplitter(this))
    , m_dView(new DetailsView(this))
    , m_flow(new Flow(this))
{
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(m_flow);
    m_splitter->addWidget(m_dView);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_splitter);
    setLayout(layout);

    m_dView->setRootIsDecorated(false);
    m_dView->setItemsExpandable(false);

    m_splitter->restoreState(Store::config.views.flowSize);

    connect(m_flow, SIGNAL(centerIndexChanged(QModelIndex)), this, SLOT(flowCurrentIndexChanged(QModelIndex)));
    connect(m_flow, SIGNAL(centerIndexChanged(QModelIndex)), Ops::absWinFor<MainWindow *>(this)->infoWidget(), SLOT(hovered(QModelIndex)));
    connect(m_splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(saveSplitter()));

    m_splitter->setHandleWidth(style()->pixelMetric(QStyle::PM_SplitterWidth));
}

FlowView::~FlowView(){}

void
FlowView::saveSplitter()
{
    Store::config.views.flowSize = m_splitter->saveState();
}

FS::Model
*FlowView::model()
{
    return static_cast<FS::Model*>(m_dView->model());
}

void
FlowView::setModel(QAbstractItemModel *model)
{
    m_dView->setModel(model);
    m_flow->setModel(model);
}

void
FlowView::setRootIndex(const QModelIndex &rootIndex)
{
    m_dView->setRootIndex(rootIndex);
    m_flow->setRootIndex(rootIndex);
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
FlowView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_dView->setSelectionModel(selectionModel);
    m_flow->setSelectionModel(selectionModel);
    connect(m_dView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,SLOT(treeCurrentIndexChanged(QItemSelection,QItemSelection)));
}

QModelIndex FlowView::rootIndex() { return m_dView->rootIndex(); }
QModelIndex FlowView::currentIndex() { return m_dView->currentIndex(); }
void FlowView::addActions(QList<QAction *> actions) { m_dView->addActions(actions); }
void FlowView::flowCurrentIndexChanged(const QModelIndex &index) { m_dView->scrollTo(index, QAbstractItemView::EnsureVisible); }
