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

QMap<QUrl, int> ColumnsWidget::m_widthMap;

ColumnsWidget::ColumnsWidget(QWidget *parent)
    : QScrollArea(parent)
    , m_viewport(new QFrame(this))
    , m_viewLay(new QHBoxLayout(m_viewport))
    , m_container(static_cast<ViewContainer *>(parent))
    , m_currentView(0)
    , m_rootIndex(QModelIndex())
    , m_model(0)
    , m_slctModel(0)
    , m_isResizingColumns(false)
{
    setWidget(m_viewport);
    m_viewLay->setContentsMargins(0, 0, 0, 0);
    m_viewLay->setSpacing(0);
    m_viewLay->setSizeConstraint(QLayout::SetMinimumSize);
    m_viewLay->addStretch();
    setWidgetResizable(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
    m_viewport->setAutoFillBackground(true);
    m_viewport->setBackgroundRole(QPalette::Base);
    m_viewport->setFrameStyle(0);
    connect(MainWindow::currentWindow(), SIGNAL(settingsChanged()), this, SLOT(reconnectViews()));
    connect(horizontalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(showCurrent()));
    setCursor(Qt::ArrowCursor);
}

void
ColumnsWidget::setModel(QAbstractItemModel *model)
{
//    qDebug() << model << static_cast<FileProxyModel *>(model)->fsModel();
    m_model = static_cast<FS::Model *>(model);
}

QModelIndex ColumnsWidget::currentIndex() { return m_currentView->currentIndex(); }
ColumnsView *ColumnsWidget::currentView() { return m_currentView; }
void ColumnsWidget::scrollTo(const QModelIndex &index) { if (m_currentView) m_currentView->scrollTo(index); }
void ColumnsWidget::edit(const QModelIndex &index) { if (m_currentView) m_currentView->edit(index); }

void
ColumnsWidget::connectView(ColumnsView *view)
{
    view->disconnect();
    connect(view, SIGNAL(opened(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(dirActivated(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(entered(QModelIndex)), m_container, SIGNAL(entered(QModelIndex)));
    connect(view, SIGNAL(newTabRequest(QModelIndex)), m_container, SLOT(genNewTabRequest(QModelIndex)));
    connect(view, SIGNAL(viewportEntered()), m_container, SIGNAL(viewportEntered()));
    connect(view, SIGNAL(focusRequest(ColumnsView*)), this, SLOT(setCurrentView(ColumnsView*)));
}

void
ColumnsWidget::reconnectViews()
{
    foreach (ColumnsView *view, m_map.values())
        connectView(view);  
}

void
ColumnsWidget::clearFrom(const QModelIndexList &list)
{
    m_currentView=0;
    foreach (const QModelIndex &index, m_map.keys())
        if (!list.contains(index))
        {
            int at = m_viewLay->indexOf(m_map.take(index));
            QLayoutItem *item = m_viewLay->takeAt(at);
            item->widget()->deleteLater();
            delete item;
        }
}

void
ColumnsWidget::setRootIndex(const QModelIndex &index)
{
    m_rootIndex = index;
    QModelIndexList list;
    QModelIndex idx = index;
    while (idx.isValid())
    {
        list.prepend(idx);
        idx=idx.parent();
    }
    if (list.size() > 1)
        list.removeFirst();
    clearFrom(list);

    for (int i=0; i<list.size(); ++i)
    {
        const QModelIndex &index = list.at(i);
        if (m_map.contains(index))
        {
            if (i+1<list.size())
                m_map.value(index)->setActiveFileName(list.at(i+1).data().toString());
            else
                m_map.value(index)->setActiveFileName(QString());
            continue;
        }
        ColumnsView *view = new ColumnsView(this, m_model, index);
        if (i+1<list.size())
            view->setActiveFileName(list.at(i+1).data().toString());
        else
            view->setActiveFileName(QString());
        connectView(view);
        view->setSelectionModel(m_slctModel);
        m_map.insert(index, view);
        m_viewLay->insertWidget(m_viewLay->count()-1, view);
    }
    m_currentView = m_map.value(index, 0);
}

void
ColumnsWidget::showCurrent()
{
    if (!m_isResizingColumns)
        ensureWidgetVisible(currentView(), m_currentView?m_currentView->width():0);
}

void
ColumnsWidget::setCurrentView(ColumnsView *view)
{
    if (!m_map.values().contains(view))
        return;
    m_currentView = view;
    emit currentViewChagned(view);
}

void
ColumnsWidget::showEvent(QShowEvent *e)
{
    QScrollArea::showEvent(e);
    ensureWidgetVisible(m_map.value(m_rootIndex, 0));
}

void
ColumnsWidget::wheelEvent(QWheelEvent *e)
{
    if (e->orientation() == Qt::Vertical && e->modifiers() == Qt::ControlModifier)
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()+(-e->delta()/6/*>0?-1:1*/));
}
