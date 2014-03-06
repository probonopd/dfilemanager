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

static QPixmap resizePix(int size, const QColor &color)
{
    QPixmap pix(size, size);
    if (size%2)
        --size;
    int half = size>>1, q = half>>1;
    pix.fill(Qt::transparent);
    QPainter pt(&pix);
//    pt.translate(0.5f, 0.5f);
    pt.setRenderHint(QPainter::Antialiasing);
    pt.setBrush(color);
    pt.setPen(color);
    static const int points[] = { 0,half, half,0, size,half, half,size };
    QPolygon pol(4, points);
    pt.drawPolygon(pol);
    pt.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    pt.setPen(Qt::NoPen);
    QRect r(half-q, 0, q*2, size);
    pt.drawRect(r);
    pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
    int height = half-qMax(1, size/8);
    r.setTop(r.top()+height);
    r.setBottom(r.bottom()-height);
    pt.drawRect(r);
    pt.end();
    return pix;
}

ResizeCorner::ResizeCorner(QWidget *parent)
    : QWidget(parent)
    , m_managed(static_cast<ColumnsView *>(parent))
    , m_hasPress(false)
    , m_hasHover(false)
    , m_prevPos(-1)
{
    setCursor(Qt::SizeHorCursor);
    setAttribute(Qt::WA_Hover);
    setAttribute(Qt::WA_MouseTracking);
    m_managed->installEventFilter(this);
    QStyleOptionSlider opt;
    opt.initFrom(m_managed);
    int sz = style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, m_managed->verticalScrollBar());
    setFixedSize(sz, sz);
    QTimer::singleShot(0, this, SLOT(postConstructor()));
}

void
ResizeCorner::postConstructor()
{
    m_pix[0] = resizePix(size().width(), palette().color(foregroundRole()));
    m_pix[1] = resizePix(size().width(), palette().color(QPalette::Highlight));
    update();
}

void
ResizeCorner::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setOpacity(m_hasHover||m_hasPress?1.0f:0.5f);
    p.drawTiledPixmap(rect(), m_pix[m_hasPress]);
    p.end();
}

void
ResizeCorner::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    m_managed->m_columnsWidget->m_isResizingColumns=true;
    m_hasPress=true;
    update();
    m_prevPos=e->globalPos().x();
}

void
ResizeCorner::mouseReleaseEvent(QMouseEvent *e)
{
    e->accept();
    m_managed->m_columnsWidget->m_isResizingColumns=false;
    m_hasPress=false;
    update();
}

void
ResizeCorner::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();
    if (!m_hasPress)
        return;

    int diff = e->globalPos().x()-m_prevPos;
    int newWidth = m_managed->width()+diff;
    if (newWidth < 64)
        return;
    move(pos().x()+diff, pos().y());
    m_managed->setFixedWidth(newWidth);
    m_managed->m_columnsWidget->m_widthMap.insert(m_managed->rootIndex().data(FS::Url).toUrl(), newWidth);
    m_prevPos = e->globalPos().x();
}

bool
ResizeCorner::eventFilter(QObject *o, QEvent *e)
{
    if (o != m_managed)
        return QWidget::eventFilter(o, e);

    switch (e->type())
    {
    case QEvent::Resize:
    {
        QResizeEvent *re = static_cast<QResizeEvent *>(e);
        if (!m_hasPress)
            move(re->size().width()-this->width(), re->size().height()-this->height());
        break;
    }
    case QEvent::Show:
        show();
        if (!m_hasPress)
            move(m_managed->width()-this->width(), m_managed->height()-this->height());
        break;
    case QEvent::Hide:
        hide();
        break;
    case QEvent::MouseButtonPress:
        if (geometry().contains(static_cast<QMouseEvent *>(e)->pos()))
        {
            QCoreApplication::sendEvent(this, e);
            return true;
        }
        break;
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
        if (m_hasPress)
        {
            QCoreApplication::sendEvent(this, e);
            return true;
        }
        break;
    }
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    {
        QHoverEvent *ev = static_cast<QHoverEvent *>(e);
        if (rect().contains(mapFrom(m_managed, ev->pos())))
        {
            if (!QApplication::overrideCursor())
                QApplication::setOverrideCursor(Qt::SplitHCursor);
            m_hasHover=true;
            update();
        }
        else
        {
            if (QApplication::overrideCursor()&&!m_hasPress)
                QApplication::restoreOverrideCursor();
            m_hasHover=false;
            update();
        }
        break;
    }
    default: break;
    }
    return QWidget::eventFilter(o, e);
}

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
        if (index.data().toString() == m_view->activeFileName() && !(option.state & QStyle::State_MouseOver))
        {
            painter->setOpacity(0.5f);
            const_cast<QStyleOptionViewItem *>(&option)->state |= QStyle::State_MouseOver;
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, m_view);
            const_cast<QStyleOptionViewItem *>(&option)->state &= ~QStyle::State_MouseOver;
            painter->setOpacity(1.0f);
        }
        FileItemDelegate::paint(painter, option, index);
        if (m_model->isDir(index))
        {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QRect r(0,0,7,7);
            r.moveCenter(m_view->expanderRect(index).center());
            QColor arcol = (option.state & QStyle::State_Selected)?option.palette.color(QPalette::HighlightedText):option.palette.color(QPalette::Text);
            if (option.state & QStyle::State_MouseOver && !(option.state & QStyle::State_Selected))
                arcol = option.palette.color(QPalette::Text);
            else if (index.data().toString() != m_view->activeFileName())
                arcol = Ops::colorMid(option.palette.color(QPalette::Base), arcol);

//            painter->setPen(QPen(arcol, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setPen(Qt::NoPen);
            painter->setBrush(arcol);
            const int points[] = { r.left(),r.top(), r.left(),r.bottom(), r.right(),r.center().y() };
            painter->drawPolygon(QPolygon(3, points));
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

ColumnsView::ColumnsView(QWidget *parent, FS::Model *model, const QModelIndex &rootIndex)
    : QListView(parent)
    , m_columnsWidget(static_cast<ColumnsWidget *>(parent))
    , m_corner(new ResizeCorner(this))
    , m_pressPos(QPoint())
    , m_activeFile(QString())
    , m_model(0)
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
    setAttribute(Qt::WA_Hover);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setFrameStyle(0);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setContentsMargins(0, 0, 16, 0);
    setModel(model);
    setRootIndex(rootIndex);
    QScrollBar *v = verticalScrollBar();
    v->installEventFilter(this);
}

ColumnsView::~ColumnsView()
{
//    m_columnsWidget->m_widthMap.insert(rootIndex().data(FS::Url).toUrl(), width());
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
            QCoreApplication::sendEvent(m_columnsWidget, we);
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
    return QRect(viewport()->rect().right()-s, vr.top(), s, s);
}

void
ColumnsView::mouseReleaseEvent(QMouseEvent *e)
{
    const QModelIndex &index = indexAt(e->pos());
    if (!index.isValid())
        return QListView::mouseReleaseEvent(e);

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

void
ColumnsView::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);
    QScrollBar *v = verticalScrollBar();
    v->resize(v->width(), height()-m_corner->height());
}

void
ColumnsView::showEvent(QShowEvent *e)
{
    QListView::showEvent(e);
    QScrollBar *v = verticalScrollBar();
    v->resize(v->width(), height()-m_corner->height());
}
