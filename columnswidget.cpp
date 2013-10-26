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

ColumnsWidget::ColumnsWidget(QWidget *parent)
    : QScrollArea(parent)
    , m_viewport(new QFrame(this))
    , m_viewLay(new QHBoxLayout(m_viewport))
    , m_container(static_cast<ViewContainer *>(parent))
    , m_currentView(0)
    , m_rootIndex(QModelIndex())
    , m_visited(QStringList())
    , m_rootList(QStringList())
    , m_model(0)
    , m_slctModel(0)
{
    setWidget(m_viewport);
    m_viewLay->setContentsMargins(0, 0, 0, 0);
    m_viewLay->setSpacing(0);
    m_viewLay->setSizeConstraint(QLayout::SetMinimumSize);
    m_viewLay->addStretch();
    setWidgetResizable(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_viewport->setAutoFillBackground(true);
    m_viewport->setBackgroundRole(QPalette::Base);
    m_viewport->setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
    setFrameStyle(0);
    connect( MainWindow::currentWindow(), SIGNAL(settingsChanged()), this, SLOT(reconnectViews()) );
    connect( horizontalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(showCurrent()) );
    setCursor(Qt::ArrowCursor);
}

void
ColumnsWidget::setModel(QAbstractItemModel *model)
{
//    qDebug() << model << static_cast<FileProxyModel *>(model)->fsModel();
    m_model = static_cast<FileSystemModel *>(model);
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
    connect(view, SIGNAL(activated(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(dirActivated(QModelIndex)), m_container, SLOT(activate(QModelIndex)));
    connect(view, SIGNAL(entered(QModelIndex)), m_container, SIGNAL(entered(QModelIndex)));
    connect(view, SIGNAL(newTabRequest(QModelIndex)), m_container, SLOT(genNewTabRequest(QModelIndex)));
    connect(view, SIGNAL(viewportEntered()), m_container, SIGNAL(viewportEntered()));
    connect(view, SIGNAL(focusRequest(ColumnsView*)), this, SLOT(setCurrentView(ColumnsView*)));
    connect(view, SIGNAL(expandRequest(QModelIndex)), this, SLOT(expand(QModelIndex)));
}

void
ColumnsWidget::reconnectViews()
{
    foreach ( ColumnsView *view, m_columns )
        connectView(view);
}

void
ColumnsWidget::clear()
{
    m_columns.clear();
    while ( !m_viewLay->isEmpty() )
    {
        QLayoutItem *item = m_viewLay->takeAt(0);
        delete item->widget();
        delete item;
    }
}

bool
ColumnsWidget::insideRoot(const QModelIndex &index)
{
    QModelIndex idx = index;
    while (idx.isValid())
    {
        if ( idx == m_rootIndex )
            return true;
        else
            idx = idx.parent();
    }
    return false;
}

QStringList
ColumnsWidget::fromRoot()
{
    QModelIndex idx = m_rootIndex;
    QStringList list;
    while (idx.isValid())
    {
        list.prepend(m_model->filePath(idx));
        idx = idx.parent();
    }
    return list;
}

ColumnsView
*ColumnsWidget::viewFor(const QString &path)
{
    ColumnsView *view = 0;
    int i = at(path);

    if ( isValid(i) )
    {
        view = column(i);
        if ( view->rootPath() != path )
            view->setRootPath(path);
        if ( !view->activeFileName().isEmpty() )
            view->setActiveFileName(QString());
    }
    else
    {
        view = new ColumnsView(this, m_model, path);
        connectView(view);
        view->setSelectionModel(m_slctModel);
        view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
        m_columns.insert(i, view);
        m_viewLay->insertWidget(i, view);
    }
    return view;
}

void
ColumnsWidget::setRootIndex(const QModelIndex &index)
{
    const QString &rootPath = m_model->filePath(index);
    if ( rootPath.isEmpty() )
        return;

    clear();

    ColumnsView *view = new ColumnsView(this, m_model, rootPath);
    connectView(view);
    view->setSelectionModel(m_slctModel);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    m_columns << view;
    m_viewLay->insertWidget(m_viewLay->count()-1, view);
    setCurrentView(view);

    if ( isValid(rootPath) )
        setCurrentView(column(rootPath));
    showCurrent();
}

void ColumnsWidget::showCurrent() { ensureWidgetVisible(currentView()); }

void
ColumnsWidget::expand(const QModelIndex &index)
{
    const QString &dirPath(m_model->filePath(index));
    int i = m_columns.indexOf(currentView());
    if ( m_viewLay->count() > ++i )
    {
        while (QLayoutItem *item = m_viewLay->itemAt(i))
        {
            if ( item->spacerItem() )
                break;
            m_columns.removeOne((ColumnsView *)item->widget());
            m_viewLay->removeItem(item);
            delete item->widget();
            delete item;
        }
    }
    ColumnsView *view = new ColumnsView(this, m_model, dirPath);
    connectView(view);
    view->setSelectionModel(m_slctModel);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    m_columns << view;
    m_viewLay->insertWidget(m_viewLay->count()-1, view);
    setCurrentView(view);
}

void
ColumnsWidget::setCurrentView( ColumnsView *view )
{
    if ( !m_columns.contains(view) )
        return;
    m_currentView = view;
    emit currentViewChagned(view);
    if ( !view->hasFocus() )
        view->setFocus();
//    if ( m_fsModel->index(m_fsModel->rootPath()) != view->rootIndex() )
//        m_fsModel->setRootPath(m_fsModel->filePath(view->rootIndex()));
}

void
ColumnsWidget::showEvent(QShowEvent *e)
{
    QScrollArea::showEvent(e);
    if ( isValid(m_model->filePath(m_rootIndex)) )
        ensureWidgetVisible(column(m_model->filePath(m_rootIndex)));
}

void
ColumnsWidget::wheelEvent(QWheelEvent *e)
{
    if ( e->orientation() == Qt::Vertical && e->modifiers() == Qt::ControlModifier )
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()+(-e->delta()/6/*>0?-1:1*/));
}
