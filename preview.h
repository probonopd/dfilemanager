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


#ifndef PREVIEW_H
#define PREVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsItemAnimation>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsProxyWidget>
#include <QGraphicsEffect>
#include <QItemSelectionModel>
#include <QHash>
#include <QMouseEvent>
#include <QGLWidget>
#include <QScrollBar>
#include <QList>
#include <QTimeLine>
#include "filesystemmodel.h"
#include "thumbsloader.h"

namespace DFM
{

#define SIZE 258.0f
#define RECT QRectF(0.0f, 0.0f, SIZE, SIZE)
class PreView;

class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    inline explicit ScrollBar( const Qt::Orientation o, QWidget *parent = 0 ) : QScrollBar(o, parent) {}
protected:
    void paintEvent(QPaintEvent *);
};

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    inline explicit GraphicsScene( const QRectF &rect = QRectF(), QWidget *parent = 0) : QGraphicsScene(rect, parent), m_preview(qobject_cast<PreView *>(parent)) {}
    inline PreView *preView() { return m_preview; }
    inline void setForegroundBrush( const QBrush &brush ) { m_fgBrush = brush; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    PreView *m_preview;
    QBrush m_fgBrush;
};

class RootItem : public QGraphicsPixmapItem
{
public:
    inline explicit RootItem(QGraphicsScene *scene = 0) : QGraphicsPixmapItem() { scene->addItem(this); }
    QRectF boundingRect() { return scene()->sceneRect(); }
};

/* PIXMAP
 * ITEM
 */

class PixmapItem : public QGraphicsItem
{
public:
    PixmapItem( GraphicsScene *scene = 0, QGraphicsItem *parent = 0 );
    void transform( const float angle, const Qt::Axis axis, const float xscale = 1.0f, const float yscale = 1.0f );
    QRectF boundingRect() const { return RECT; }
    inline void saveX() { m_savedX = pos().x(); }
    inline float savedX() { return m_savedX; }
    void updatePixmaps();
    QPainterPath shape() const { return m_shape; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void updateShape();

private:
    QPixmap m_pix[2];
    GraphicsScene *m_scene;
    PreView *m_preView;
    float m_rotate, m_savedX;
    bool m_isUpdateingPixmaps;
    QPainterPath m_shape;
};

/* THE
 * VIEW
 */

class PreView : public QGraphicsView
{
    Q_OBJECT
public:
    enum Pos { Prev = 0, New = 1 };
    explicit PreView(QWidget *parent = 0);
    void setModel( QAbstractItemModel *model );
    inline void setSelectionModel( QItemSelectionModel *model ) { m_selectionModel = model; }
    void setCenterIndex( const QModelIndex &index );
    void showCenterIndex( const QModelIndex &index );
    inline void animateCenterIndex( const QModelIndex &index ) { m_scrollBar->setValue(index.row()); }
    inline FileSystemModel *model() { return m_model; }
    QModelIndex indexOfItem(PixmapItem *item);
    inline bool isAnimating() { return bool(m_timeLine->state() == QTimeLine::Running); }
    
signals:
    void centerIndexChanged( const QModelIndex &centerIndex );
    
public slots:
    void setRootIndex( const QModelIndex &rootIndex );

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEvent *e);
    void populate(const int start, const int end);
    void prepareAnimation();
    inline bool isValidRow( const int row ) { return bool(row > -1 && row < m_items.count()); }
    void correctItemsPos(const int leftStart, const int rightStart);
    void showPrevious();
    void showNext();
    inline int validate(const int row) { return qBound(0, row, m_items.count()-1); }

    template<typename T>T itemAtAs(const QPoint &pos){return dynamic_cast<T>(itemAt(pos));}

private slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void reset();
    void clear();
    void animStep( const qreal value );
    void updateItemsPos();
    void scrollBarMoved( const int value );
    void continueIf();

private:
    friend class PixmapItem;
    GraphicsScene *m_scene;
    FileSystemModel *m_model;
    QModelIndex m_centerIndex, m_prevCenter, m_savedCenter;
    QPersistentModelIndex m_rootIndex;
    int m_row, m_nextRow, m_newRow, m_savedRow;
    float m_y, m_x, m_perception, m_xpos;
    bool m_wantsDrag, m_hasZUpdate;
    QList<PixmapItem *> m_items;
    QGraphicsItemAnimation *m_anim[2];
    QTimeLine *m_timeLine;
    QGraphicsItem *m_pressed;
    QGraphicsSimpleTextItem *m_textItem;
    RootItem *m_rootItem;
    QGraphicsProxyWidget *m_gfxProxy;
    ScrollBar *m_scrollBar;
    QPointF m_pressPos;
    QItemSelectionModel *m_selectionModel;
    QString m_centerFile, m_rootPath;
};

}

#endif // PREVIEW_H
