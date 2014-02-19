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

class ColumnsDelegate : public FileItemDelegate
{
public:
    explicit ColumnsDelegate(ColumnsView *parent = 0)
        : FileItemDelegate(parent)
        , m_view(parent)
        , m_model(static_cast<FS::Model *>(m_view->model())){}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (!index.data().isValid()||!option.rect.isValid())
            return;
        if (index.data().toString() == m_view->activeFileName())
        {
            QColor h = m_view->palette().color(QPalette::Highlight);
            h.setAlpha(64);
            painter->fillRect(option.rect, h);
        }
        FileItemDelegate::paint(painter, option, index);
        if (m_model->isDir(index) && m_model->url(index).scheme() == "file")
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect r(0,0,7,7);
            r.moveCenter(m_view->expanderRect(index).center());
            QPolygon p;
            p.putPoints(0, 4, r.left(), r.top(), r.left(), r.bottom(), r.right(), r.center().y(), r.left(), r.top());
            QPainterPath path;
            QColor arcol = (option.state & QStyle::State_Selected)?option.palette.color(QPalette::HighlightedText):option.palette.color(QPalette::Text);
            if (index.data().toString() != m_view->activeFileName())
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
    FS::Model *m_model;
};

ColumnsView::ColumnsView(QWidget *parent, QAbstractItemModel *model, const QModelIndex &rootIndex)
    : QListView(parent)
    , m_parent(static_cast<ColumnsWidget *>(parent))
    , m_pressPos(QPoint())
    , m_activeFile(QString())
    , m_model(0)
    , m_width(0)
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
    setSelectionBehavior(QAbstractItemView::SelectRows);
    if (FS::Model *fsModel = qobject_cast<FS::Model *>(model))
    {
        setModel(fsModel);
        setRootIndex(rootIndex);
    }
    verticalScrollBar()->installEventFilter(this);
}

bool
ColumnsView::eventFilter(QObject *o, QEvent *e)
{
    // grrrrr, preventing wheel events when control pressed
    // is not enough... the scrollbar still gets them.
    if (o == verticalScrollBar() && e->type() == QEvent::Wheel)
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(e);
        if (we->modifiers() == Qt::ControlModifier)
        {
            QCoreApplication::sendEvent(m_parent, we);
            return true;
        }
    }
    else
        return QListView::eventFilter(o, e);
}

void
ColumnsView::keyPressEvent(QKeyEvent *event)
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
    ViewBase::keyPressEvent(event);
    QListView::keyPressEvent(event);
}

void
ColumnsView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu popupMenu;
    if (Store::customActions().count())
        popupMenu.addMenu(Store::customActionsMenu());
    popupMenu.addActions(ViewContainer::rightClickActions());
    const QFileInfo &file = m_model->fileInfo(indexAt(event->pos()));
    QMenu openWith(tr("Open With"), this);
    if (file.exists())
        openWith.addActions(Store::openWithActions(file.filePath()));
    foreach (QAction *action, actions())
    {
        popupMenu.addAction(action);
        if (action->objectName() == "actionDelete")
            popupMenu.insertSeparator(action);
        if (action->objectName() == "actionCustomCmd")
            popupMenu.insertMenu(action, &openWith);
    }
    popupMenu.exec(event->globalPos());
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
    if (!index.isValid())
        return QListView::mouseReleaseEvent(e);

    if (expanderRect(index).contains(e->pos())
         && index == indexAt(m_pressPos))
    {
        emit expandRequest(index);
        setActiveFileName(index.data().toString());
        e->accept();
        return;
    }

    if (Store::config.views.singleClick
         && !e->modifiers()
         && e->button() == Qt::LeftButton
         && m_pressPos == e->pos()
         && state() == NoState)
    {
        emit opened(index);
        e->accept();
        return;
    }

    if (!Store::config.views.singleClick
         && e->modifiers() == Qt::NoModifier
         && e->button() == Qt::LeftButton
         && m_pressPos == e->pos()
         && m_model->isDir(index))
        emit dirActivated(index);
    else if (e->button() == Qt::MiddleButton
              && m_pressPos == e->pos()
              && !e->modifiers())
        emit newTabRequest(index);
    else
        QListView::mouseReleaseEvent(e);
}

void
ColumnsView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    m_model = qobject_cast<FS::Model *>(model);
    setItemDelegate(new ColumnsDelegate(this));
}

void
ColumnsView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() == Qt::ControlModifier)
    {
        e->ignore();
        return;
    }
    QListView::wheelEvent(e);
}

void
ColumnsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QListView::mouseDoubleClickEvent(event);
    const QModelIndex &index = indexAt(event->pos());
    if (index.isValid()
            && !Store::config.views.singleClick
            && !event->modifiers()
            && event->button() == Qt::LeftButton
            && state() == NoState)
        emit opened(index);
}
