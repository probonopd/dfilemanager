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


#include "columnsview.h"
#include "viewcontainer.h"
#include "mainwindow.h"
#include "filesystemmodel.h"
#include "operations.h"
#include <QPainter>

using namespace DFM;

class ColumnsDelegate : public QStyledItemDelegate
{
public:
    explicit ColumnsDelegate(QWidget *parent = 0)
        : QStyledItemDelegate(parent)
        , m_view(static_cast<ColumnsView *>(parent)){}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if ( index.data().toString() == m_view->activeFileName() )
        {
            QColor h = m_view->palette().color(QPalette::Highlight);
            h.setAlpha(64);
            painter->fillRect(option.rect, h);
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        int w = option.fontMetrics.width(index.data().toString());
        int h = 16;
        return QSize(w, h);
    }

private:
    ColumnsView *m_view;
};

ColumnsView::ColumnsView(QWidget *parent, QAbstractItemModel *model, const QString &rootPath)
    : QListView(parent)
    , m_parent(static_cast<ColumnsWidget *>(parent))
    , m_pressPos(QPoint())
    , m_activeFile(QString())
    , m_rootPath(QString())
    , m_model(0)
    , m_width(0)
    , m_isSorted(false)
    , m_fsWatcher(new QFileSystemWatcher(this))
    , m_sortModel(0)
{
    setItemDelegate(new ColumnsDelegate(this));
    setViewMode(QListView::ListMode);
    setResizeMode(QListView::Adjust);
    setIconSize(QSize(16, 16));
    setUniformItemSizes(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::SelectedClicked|QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionRectVisible(true);
    setMouseTracking(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setTextElideMode(Qt::ElideNone);
//    connect( m_parent, SIGNAL(currentViewChagned(ColumnsView*)), viewport(), SLOT(update()) );
    if ( FileSystemModel *fsModel = qobject_cast<FileSystemModel *>(model) )
    {
        connect(fsModel, SIGNAL(rowsRemoved(QModelIndex,int ,int)), this, SLOT(rowsRemoved(QModelIndex,int,int)));
        connect(fsModel, SIGNAL(fileRenamed(QString,QString,QString)), this, SLOT(fileRenamed(QString,QString,QString)));
        connect(fsModel, SIGNAL(directoryLoaded(QString)), this, SLOT(dirLoaded(QString)));
        setModel(fsModel);
        if ( QFileInfo(rootPath).exists() )
            setRootIndex(fsModel->index(rootPath));
    }
    //as we are not using the filesystemmodel rootpath we will not get notified...
    connect(m_fsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(dirLoaded(QString)));
    verticalScrollBar()->installEventFilter(this);
}

bool
ColumnsView::eventFilter(QObject *o, QEvent *e)
{
    // grrrrr, preventing wheel events when control pressed
    // is not enough... the scrollbar still gets them.
    if ( o == verticalScrollBar() && e->type() == QEvent::Wheel )
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(e);
        if ( we->modifiers() == Qt::ControlModifier )
        {
            QCoreApplication::sendEvent(m_parent, we);
            return true;
        }
    }
    else
        return QListView::eventFilter(o, e);
}

void
ColumnsView::setRootPath(const QString &path)
{
    setRootIndex(m_model->index(path));
}

void
ColumnsView::setRootIndex(const QModelIndex &index)
{
    if ( !m_fsWatcher->directories().isEmpty() )
        m_fsWatcher->removePaths(m_fsWatcher->directories());
    if ( m_model )
    {
        m_isSorted = index == m_model->index(m_model->rootPath());
        m_rootPath = m_model->filePath(index);
        if ( !sanityCheckForDir() )
            return;
        m_fsWatcher->addPath(m_rootPath);
    }
    QListView::setRootIndex(index);
    updateWidth();
}

void
ColumnsView::setFilter(QString filter)
{
    for (int i = 0; i < model()->rowCount(rootIndex()); i++)
    {
        const QModelIndex &index = model()->index(i,0,rootIndex());
        if (index.data().toString().contains(filter, Qt::CaseInsensitive))
            setRowHidden(i, false);
        else
            setRowHidden(i, true);
    }
}

void
ColumnsView::keyPressEvent(QKeyEvent *event)
{
    if ( event->key() == Qt::Key_Return && event->modifiers() == Qt::NoModifier && state() != QAbstractItemView::EditingState )
    {
        if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                if ( index.column() == 0 )
                    emit activated(index);
        event->accept();
        return;
    }
    QListView::keyPressEvent(event);
}

void
ColumnsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu popupMenu;
    if ( Store::customActions().count() )
        popupMenu.addMenu(Store::customActionsMenu());
    popupMenu.addActions( ViewContainer::rightClickActions() );
    const QString &file = m_model->filePath( indexAt( event->pos() ) );
    QMenu openWith( tr( "Open With" ), this );
    openWith.addActions( Store::openWithActions( file ) );
    foreach ( QAction *action, actions() )
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
ColumnsView::mouseReleaseEvent(QMouseEvent *e)
{
    const QModelIndex &index = indexAt(e->pos());
    if ( !Store::config.views.singleClick
         && e->modifiers() == Qt::NoModifier
         && e->button() == Qt::LeftButton
         && m_pressPos == e->pos()
         && m_model->fileInfo(index).isDir() )
        emit dirActivated(index);
    else if ( e->button() == Qt::MiddleButton
              && indexAt(e->pos()).isValid()
              && m_pressPos == e->pos() )
        emit newTabRequest(index);
    else
        QListView::mouseReleaseEvent(e);
}

void
ColumnsView::paintEvent(QPaintEvent *e)
{
    QListView::paintEvent(e);
//    if ( m_parent->currentView() && m_parent->currentView() == this )
//    {
//        QPainter p(viewport());
//        p.setPen(QPen(palette().color(QPalette::Highlight), 3.0f));
//        p.drawRect(viewport()->rect().adjusted(0,0,-1,-1));
//        p.end();
//    }
}

void
ColumnsView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    m_model = qobject_cast<FileSystemModel *>(model);
//    connect( m_proxyModel, SIGNAL(sortingChanged(int,Qt::SortOrder)), this, SLOT(sortingChanged(int,Qt::SortOrder)) );
}

void
ColumnsView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QListView::rowsInserted(parent, start, end);
    if ( parent != rootIndex() )
        return;
    int wbefore = m_width;
    for ( int i = start; i<=end; ++i )
    {
        const QModelIndex &index = m_model->index(i, 0, parent);
        if ( !index.isValid() )
            continue;
        const int W =  fontMetrics().boundingRect( index.data().toString() ).width();
        if ( W > m_width )
            m_width = W;
    }
    if ( m_width > wbefore )
    {
        int w = m_width;
        w+=22; //icon
        w+=style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, this);
        setFixedWidth(w);
    }
}

void
ColumnsView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if ( !rootIndex().isValid() )
    {
        emit pathDeleted(this);
        hide();
    }
    if ( parent == rootIndex() )
        updateWidth();
}

void
ColumnsView::sortingChanged(const int column, const Qt::SortOrder order)
{
//    const QString &oldPath = m_fsModel->rootPath();
//    m_fsModel->blockSignals(true);
//    m_fsModel->setRootPath(m_rootPath);
//    m_fsModel->sort(column, order);
//    m_fsModel->setRootPath(oldPath);
//    m_fsModel->blockSignals(false);
}

void
ColumnsView::dirLoaded(const QString &dir)
{
    if ( !sanityCheckForDir() )
        return;
    if ( (dir != m_rootPath || m_isSorted) && sender()!=m_fsWatcher )
        return;

    m_isSorted = true;
//    sortingChanged(m_proxyModel->sortingColumn(), m_proxyModel->sortingOrder());
    viewport()->update();
}

void
ColumnsView::fileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
    if ( path != m_rootPath )
        return;

    int w = fontMetrics().boundingRect(newName).width();
    if ( w > m_width )
    {
        m_width = w ;
        w+=22; //icon
        w+=style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, this);
        setFixedWidth(w);
    }
    else
        updateWidth();
}

void
ColumnsView::updateWidth()
{
    if ( !sanityCheckForDir() )
        return;
    if ( !m_model )
        return;
    QStringList list = QDir(m_model->filePath(rootIndex())).entryList(QDir::AllEntries|QDir::AllDirs|QDir::NoDotAndDotDot|QDir::System);
    int w = 0;
    while ( !list.isEmpty() )
    {
        const int W = fontMetrics().boundingRect(list.takeFirst()).width();
        if ( W > w )
            w = W;
    }
    w+=22; //icon
    w+=style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, this);
    setFixedWidth(qMax(64, w));
}

bool
ColumnsView::sanityCheckForDir()
{
    if ( !m_rootPath.isEmpty() && !QFileInfo(m_rootPath).exists() )
    {
        emit pathDeleted(this);
        hide();
        if ( m_model )
        {
            QString dir = m_rootPath;
            while ( !QFileInfo(dir).exists() )
            {
                if ( m_fsWatcher->directories().contains(dir) )
                    m_fsWatcher->removePath(dir);
                dir = QFileInfo(dir).path();
            }
            if ( m_model->rootPath() != dir )
                m_model->setRootPath(dir);
            return false;
        }
        return false;
    }
    return true;
}

void
ColumnsView::wheelEvent(QWheelEvent *e)
{
    if ( e->modifiers() == Qt::ControlModifier )
    {
        e->ignore();
        return;
    }
    QListView::wheelEvent(e);
}
