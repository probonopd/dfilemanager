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
#include <QTextEdit>
#include <QMessageBox>

using namespace DFM;

class ColumnsDelegate : public QStyledItemDelegate
{
public:
    explicit ColumnsDelegate(ColumnsView *parent = 0)
        : QStyledItemDelegate(parent)
        , m_view(parent)
        , m_model(static_cast<FileSystemModel *>(m_view->model())){}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QTextEdit *editor = new QTextEdit(parent);
        editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return editor;
    }
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        QTextEdit *edit = qobject_cast<QTextEdit *>(editor);
        FileSystemModel *fsModel = static_cast<FileSystemModel *>(model);
        if ( !edit||!fsModel )
            return;
        FileSystemModel::Node *node = fsModel->fromIndex(index);
        const QString &newName = edit->toPlainText();
        if ( node->name() == newName )
            return;
        if ( !node->rename(newName) )
            QMessageBox::warning(MainWindow::window(edit), "Failed to rename", QString("%1 to %2").arg(node->name(), newName));
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if ( !index.data().isValid()||!option.rect.isValid() )
            return;
        if ( index.data().toString() == m_view->activeFileName() )
        {
            QColor h = m_view->palette().color(QPalette::Highlight);
            h.setAlpha(64);
            painter->fillRect(option.rect, h);
        }
        QStyledItemDelegate::paint(painter, option, index);
        if ( m_model->isDir(index) )
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect r(0,0,7,7);
            r.moveCenter(m_view->expanderRect(index).center());
            QPolygon p;
            p.putPoints(0, 4, r.left(), r.top(), r.left(), r.bottom(), r.right(), r.center().y(), r.left(), r.top());
            QPainterPath path;
            QColor arcol = (option.state & QStyle::State_Selected)?option.palette.color(QPalette::HighlightedText):option.palette.color(QPalette::Text);
            if ( index.data().toString() != m_view->activeFileName() )
                arcol = Ops::colorMid(option.palette.color(QPalette::Base), arcol);
            path.addPolygon(p);
            painter->setPen(QPen(arcol, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(arcol);
            painter->drawPath(path);
            painter->restore();
        }
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        int w = option.fontMetrics.width(index.data().toString());
        int h = 16;
        return QSize(w, h);
    }

private:
    ColumnsView *m_view;
    FileSystemModel *m_model;
};

ColumnsView::ColumnsView(QWidget *parent, QAbstractItemModel *model, const QString &rootPath)
    : QListView(parent)
    , m_parent(static_cast<ColumnsWidget *>(parent))
    , m_pressPos(QPoint())
    , m_activeFile(QString())
    , m_rootPath(QString())
    , m_model(0)
    , m_width(0)
    , m_fsWatcher(new QFileSystemWatcher(this))
    , m_sortModel(0)
{
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
    setFrameStyle(0);
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
    if ( event->key() == Qt::Key_Escape )
        clearSelection();
    if ( event->key() == Qt::Key_Return
         && event->modifiers() == Qt::NoModifier
         && state() == NoState )
    {
        if ( selectionModel()->selectedIndexes().count() )
            foreach ( const QModelIndex &index, selectionModel()->selectedIndexes() )
                if ( !index.column() )
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

QRect
ColumnsView::expanderRect(const QModelIndex &index)
{
    QRect vr = visualRect(index);
    const int s = vr.height();
    return QRect(vr.right()-s, vr.top(), s, s);
}

void
ColumnsView::mouseReleaseEvent(QMouseEvent *e)
{
    const QModelIndex &index = indexAt(e->pos());
    if ( !index.isValid() )
        return QListView::mouseReleaseEvent(e);

    if ( expanderRect(index).contains(e->pos())
         && index == indexAt(m_pressPos) )
    {
        emit expandRequest(index);
        setActiveFileName(index.data().toString());
        e->accept();
        return;
    }

    if ( Store::config.views.singleClick
         && !e->modifiers()
         && e->button() == Qt::LeftButton
         && m_pressPos == e->pos()
         && state() == NoState )
    {
        emit activated(index);
        e->accept();
        return;
    }

    if ( !Store::config.views.singleClick
         && e->modifiers() == Qt::NoModifier
         && e->button() == Qt::LeftButton
         && m_pressPos == e->pos()
         && m_model->isDir(index) )
        emit dirActivated(index);
    else if ( e->button() == Qt::MiddleButton
              && m_pressPos == e->pos()
              && !e->modifiers() )
        emit newTabRequest(index);
    else
        QListView::mouseReleaseEvent(e);
}

void
ColumnsView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    m_model = qobject_cast<FileSystemModel *>(model);
    setItemDelegate(new ColumnsDelegate(this));
    connect(m_model, SIGNAL(paintRequest()), viewport(), SLOT(update()));
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
ColumnsView::dirLoaded(const QString &dir)
{
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
        w+=40; //icon && expanders...
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
    w+=40; //icon && expanders...
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
        if ( m_fsWatcher->directories().contains(m_rootPath) )
            m_fsWatcher->removePath(m_rootPath);
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
