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
    connect(m_container, SIGNAL(settingsChanged()), this, SLOT(reconnectViews()));
    connect(horizontalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(showCurrent()));
    setCursor(Qt::ArrowCursor);
}

ColumnsWidget::~ColumnsWidget()
{
    m_columns.clear();
}

void
ColumnsWidget::setModel(FS::Model *model)
{
//    qDebug() << model << static_cast<FileProxyModel *>(model)->fsModel();
    m_model = model;
}

QModelIndex
ColumnsWidget::currentIndex()
{
    if (ColumnsView *view = currentView())
        return view->currentIndex();
    return QModelIndex();
}
ColumnsView
*ColumnsWidget::currentView()
{
    if (ColumnsView *view = qobject_cast<ColumnsView *>(focusWidget()))
        return view;
    if (!m_columns.isEmpty())
    if (ColumnsView *view = m_columns.last())
        return view;
    return 0;
}
void
ColumnsWidget::scrollTo(const QModelIndex &index)
{
    if (ColumnsView *view = currentView())
        view->scrollTo(index);
}
void
ColumnsWidget::edit(const QModelIndex &index)
{
    if (ColumnsView *view = currentView())
        view->edit(index);
}

void
ColumnsWidget::connectView(ColumnsView *view)
{
    view->disconnect();
    connect(view, SIGNAL(opened(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(dirActivated(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(entered(QModelIndex)), m_container, SIGNAL(entered(QModelIndex)));
    connect(view, SIGNAL(newTabRequest(QModelIndex)), m_container, SLOT(genNewTabRequest(QModelIndex)));
    connect(view, SIGNAL(viewportEntered()), m_container, SIGNAL(viewportEntered()));
}

void
ColumnsWidget::reconnectViews()
{
    foreach (ColumnsView *view, m_columns)
        connectView(view);
}

void
ColumnsWidget::removeView(ColumnsView *view)
{
    view->deleteLater();
    if (QLayoutItem *item = m_viewLay->takeAt(m_viewLay->indexOf(view)))
        delete item;
}

void
ColumnsWidget::clear(const QModelIndexList &list)
{
    for (int i = 0; i < m_columns.count(); ++i)
    {
        ColumnsView *view = m_columns.at(i);
        const QModelIndex &index = view->rootIndex();
        const QString &viewUrl = index.data(FS::Url).toUrl().toString();
        const QString &rootUrl = m_rootIndex.data(FS::Url).toUrl().toString();

        if (!rootUrl.contains(viewUrl) || !list.contains(index))
            removeView(view);
    }
}

ColumnsView
*ColumnsWidget::column(const QModelIndex &index) const
{
    for (int i = 0; i < m_columns.count(); ++i)
    {
        ColumnsView *v = m_columns.at(i);
        if (v->rootIndex() == index)
            return v;
    }
    return 0;
}

QModelIndexList
ColumnsWidget::parents(const QModelIndex &index) const
{
    QModelIndexList list;
    QModelIndex idx = index;
    while (idx.isValid())
    {
        list.prepend(idx);
        const QString &path = idx.data(FS::FilePathRole).toString();
        if (MainWindow::currentWindow()->placesView()->hasPlace(path))
            return list;
        if (Devices::instance()->isDevice(path))
            return list;
        idx=idx.parent();
    }
    if (list.isEmpty())
        list << QModelIndex();
    return list;
}

void
ColumnsWidget::setRootIndex(const QModelIndex &index)
{
    m_rootIndex = index;
    const QModelIndexList &list = parents(index);
    clear(list);

    for (int i=0; i<list.size(); ++i)
    {
        const QModelIndex &index = list.at(i);
        const bool isntLast = i+1<list.size();
        const QString &activeName(isntLast?list.at(i+1).data().toString():QString());
        if (ColumnsView *view = column(index))
        {
            view->setActiveFileName(activeName);
            continue;
        }
        ColumnsView *view = new ColumnsView(this, m_model, index);
        view->setActiveFileName(activeName);
        connectView(view);
        view->setSelectionModel(m_slctModel);
        m_columns << view;
        m_viewLay->insertWidget(m_viewLay->count()-1, view);
    }
    if (!m_columns.isEmpty())
        m_columns.last()->setFocus();
}

void
ColumnsWidget::showCurrent()
{
    if (!m_isResizingColumns)
        if (ColumnsView *view = currentView())
            ensureWidgetVisible(view, view->width());
}

void
ColumnsWidget::showEvent(QShowEvent *e)
{
    QScrollArea::showEvent(e);
    if (ColumnsView *view = currentView())
        ensureWidgetVisible(view, view->width());
}

void
ColumnsWidget::wheelEvent(QWheelEvent *e)
{
    if (e->orientation() == Qt::Vertical && e->modifiers() == Qt::ControlModifier)
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()+(-e->delta()/6/*>0?-1:1*/));
}
