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

#include "columnswidget.h"
#include "operations.h"
#include "config.h"
#include "mainwindow.h"

using namespace DFM;

ColumnsWidget::ColumnsWidget(QWidget *parent) :
    QScrollArea(parent)
  , m_fsModel(0)
  , m_slctModel(0)
  , m_viewport(new QWidget(this))
  , m_viewLay(new QHBoxLayout(m_viewport))
  , m_container(static_cast<ViewContainer *>(parent))
  , m_currentView(0)
  , m_rootIndex(QModelIndex())
  , m_visited(QModelIndexList())
{
    setWidget(m_viewport);
//    setViewport(m_viewport);
    m_viewLay->setContentsMargins(0, 0, 0, 0);
    m_viewLay->setSpacing(0);
    m_viewLay->setSizeConstraint(QLayout::SetFixedSize);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWidgetResizable(true);
    setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Dark);
    setForegroundRole(QPalette::Light);
    connect( MainWindow::currentWindow(), SIGNAL(settingsChanged()), this, SLOT(reconnectViews()) );
    setCursor(Qt::ArrowCursor);
}

void
ColumnsWidget::setModel(FileSystemModel *model)
{
    m_fsModel = model;
    connect(m_fsModel, SIGNAL(rootPathChanged(QString)), this, SLOT(rootPathChanged(QString)));
}

QModelIndex ColumnsWidget::currentIndex() { return m_currentView->currentIndex(); }
ColumnsView *ColumnsWidget::currentView() { return m_currentView; }
void ColumnsWidget::setFilter( const QString &filter ) { if ( m_currentView ) m_currentView->setFilter(filter); }
void ColumnsWidget::scrollTo(const QModelIndex &index) { if ( m_currentView ) m_currentView->scrollTo(index); }
void ColumnsWidget::edit(const QModelIndex &index) { if ( m_currentView ) m_currentView->edit(index); }

void
ColumnsWidget::connectView(ColumnsView *view)
{
    view->disconnect();
    if ( Store::config.views.singleClick )
        connect(view, SIGNAL(clicked(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    else
        connect(view, SIGNAL(activated(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(dirActivated(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(entered(QModelIndex)), m_container, SIGNAL(entered(QModelIndex)));
    connect(view, SIGNAL(newTabRequest(QModelIndex)), m_container, SLOT(genNewTabRequest(QModelIndex)));
    connect(view, SIGNAL(viewportEntered()), m_container, SIGNAL(viewportEntered()));
    connect(view, SIGNAL(pathDeleted(ColumnsView*)), this, SLOT(deleteView(ColumnsView*)));
    connect(view, SIGNAL(focusRequest(ColumnsView*)), this, SLOT(setCurrentView(ColumnsView*)));
    connect(view, SIGNAL(showed(ColumnsView*)), this, SLOT(correctHeightForView(ColumnsView*)));
}

void
ColumnsWidget::reconnectViews()
{
    foreach ( ColumnsView *view, m_views.values() )
        connectView(view);
}

void
ColumnsWidget::clear()
{
    m_views.clear();
    while (QLayoutItem *item = m_viewLay->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
}

void
ColumnsWidget::deleteView(ColumnsView *view)
{
    if ( m_views.contains(m_views.key(view)) )
        m_views.remove(view->rootIndex());
    QLayoutItem *item = m_viewLay->takeAt(m_viewLay->indexOf(view));
    delete item->widget();
    delete item;
}

ColumnsView
*ColumnsWidget::newView(const QModelIndex &index)
{
    ColumnsView *view = new ColumnsView(this, m_fsModel, index);
    connectView(view);
    view->setSelectionModel(m_slctModel);
    m_views.insert(index, view);
    m_viewLay->addWidget(view);
    return view;
}

QModelIndexList
ColumnsWidget::fromRoot()
{
    QModelIndexList idxList;
    QModelIndex idx = m_rootIndex;
    while ( idx.isValid() )
    {
        idxList.prepend(idx);
        idx = idx.parent();
    }
    return idxList;
}

QModelIndexList
ColumnsWidget::fromLast()
{
    QModelIndexList idxList;
    if ( m_viewLay->isEmpty() )
        return idxList;
    QModelIndex idx = static_cast<ColumnsView *>(m_viewLay->itemAt(m_viewLay->count()-1)->widget())->rootIndex();
    while ( idx.isValid() )
    {
        idxList.prepend(idx);
        idx = idx.parent();
    }
    return idxList;
}

void
ColumnsWidget::setRootIndex(const QModelIndex &index)
{
    m_visited << index;
    const bool alreadyExists = fromLast().contains(index);
    m_rootIndex = index;
    if ( m_views.contains(index) )
        m_currentView = m_views.value(index);
    QModelIndexList idxList = fromRoot();

    if ( !alreadyExists )
    {
        foreach ( const QModelIndex &asdf, m_views.keys() )
            if ( idxList.contains(asdf) )
                continue;
            else
            {
                ColumnsView *view = m_views.take(asdf);
                QLayoutItem *item = m_viewLay->takeAt(m_viewLay->indexOf(view));
                delete item->widget();
                delete item;
            }
        foreach ( const QModelIndex &dirIdx, idxList )
        {
            if ( m_views.contains(dirIdx) )
                continue;
            else
                m_views.insert(dirIdx, newView(dirIdx));
        }
    }
    bool parIsVis = false;
    idxList = alreadyExists ? fromLast() : fromRoot();
    foreach ( const QModelIndex &dirIdx, idxList )
    {
        parIsVis = parIsVis||m_visited.contains(dirIdx);
        if ( m_views.contains(dirIdx) )
            m_views.value(dirIdx)->setVisible(parIsVis);
    }

    while ( !idxList.isEmpty() )
    {
        const QModelIndex &idx = idxList.takeLast();
        const QModelIndex &parent = idx.parent();
        if ( parent.isValid() && m_views.contains(parent) )
            m_views.value(parent)->setActiveFileName(m_fsModel->fileName(idx));
    }

    setCurrentView(m_views.value(index), !alreadyExists);
}

void
ColumnsWidget::rootPathChanged(const QString &rootPath)
{
    m_rootPath = rootPath;
}

void
ColumnsWidget::setCurrentView( ColumnsView *view, bool setVisibility )
{
    if ( !m_views.values().contains(view) )
        return;
    if ( m_currentView && m_currentView == view && m_currentView->hasFocus() )
        return;
    m_currentView = view;
    emit currentViewChagned(view);
    if ( !view->hasFocus() )
        view->setFocus();
    if ( m_fsModel->index(m_fsModel->rootPath()) != view->rootIndex() )
        m_fsModel->setRootPath(m_fsModel->filePath(view->rootIndex()));
    if ( setVisibility )
        ensureWidgetVisible(view, view->width());
}

void
ColumnsWidget::correctHeightForView(ColumnsView *view)
{
    int h = size().height();
    if ( horizontalScrollBar()->isVisible() )
        h -= horizontalScrollBar()->height();
    view->setFixedHeight(h);
}

void
ColumnsWidget::resizeEvent(QResizeEvent *e)
{
    QScrollArea::resizeEvent(e);

    if ( e->size().height() != e->oldSize().height() )
        foreach ( ColumnsView *view, m_views.values() )
            if ( view->isVisible() )
                correctHeightForView(view);
}

void
ColumnsWidget::showEvent(QShowEvent *e)
{
    QScrollArea::showEvent(e);
    ensureWidgetVisible(m_views.value(m_rootIndex));
}
