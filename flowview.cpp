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
#include "application.h"
#include "infowidget.h"
#include "operations.h"
#include <QImageReader>

using namespace DFM;

FlowView::FlowView(QWidget *parent) : QWidget(parent)
{
    m_splitter = new QSplitter(this);
    m_dView = new DetailsView(m_splitter);
    m_preView = new PreView(m_splitter);
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(m_preView);
    m_splitter->addWidget(m_dView);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_splitter);
    setLayout(layout);

    m_splitter->restoreState( Configuration::config.views.flowSize );

    connect( m_preView, SIGNAL(centerIndexChanged(QModelIndex)), this, SLOT(flowCurrentIndexChanged(QModelIndex)) );
    connect( m_preView, SIGNAL(centerIndexChanged(QModelIndex)), Operations::absWinFor<MainWindow *>(this)->infoWidget(), SLOT(hovered(QModelIndex)) );
    connect( m_splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(saveSplitter()) );


    m_splitter->setHandleWidth(style()->pixelMetric(QStyle::PM_SplitterWidth));
}

void
FlowView::saveSplitter()
{
    Configuration::config.views.flowSize = m_splitter->saveState();
}

void
FlowView::setModel(FileSystemModel *model)
{
    m_dView->setModel(model);
    m_preView->setModel(model);
}

void
FlowView::setRootIndex(const QModelIndex &rootIndex)
{
    if(rootIndex.isValid())
    {
        m_dView->setRootIndex(rootIndex);
        m_preView->setRootIndex(rootIndex);
    }
}

void
FlowView::treeCurrentIndexChanged(QItemSelection,QItemSelection)
{
    if (m_dView->selectionModel()->currentIndex().isValid())
    {
        m_preView->animateCenterIndex(m_dView->selectionModel()->currentIndex());
        m_dView->scrollTo(m_dView->selectionModel()->currentIndex(), QAbstractItemView::PositionAtCenter);
    }
}

void
FlowView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_dView->setSelectionModel(selectionModel);
    connect( m_dView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this,SLOT(treeCurrentIndexChanged(QItemSelection,QItemSelection)));
}
