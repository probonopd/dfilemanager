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
    inline explicit ScrollBar( const Qt::Orientation &o, QWidget *parent = 0 ) : QScrollBar(o, parent) {}
protected:
    void paintEvent(QPaintEvent *);
};

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    inline explicit GraphicsScene( const QRectF &rect = QRectF(), QWidget *parent = 0) : QGraphicsScene(rect, parent), m_preview(qobject_cast<PreView *>(parent)) {}
    inline PreView *preView() { return m_preview; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    PreView *m_preview;
};

class RootItem : public QGraphicsPixmapItem
{
public:
    inline explicit RootItem(QGraphicsScene *scene = 0) : QGraphicsPixmapItem(QPixmap(), 0, scene) {}
    inline QRectF boundingRect() { return scene()->sceneRect(); }
};

/* PIXMAP
 * ITEM
 */

class PixmapItem : public QGraphicsPixmapItem
{
public:
    inline explicit PixmapItem( const QPixmap &pix = QPixmap(), GraphicsScene *scene = 0, QGraphicsItem *parent = 0 ) :
        QGraphicsPixmapItem(pix, parent, scene),
        m_reflection(QPixmap()),
        m_scene(scene){}
    void rotate( const float &angle, const Qt::Axis &axis );
    inline void setReflection( const QPixmap &reflection ) { m_reflection = reflection; }
    inline QRectF boundingRect() { return RECT; }
    inline void saveX() { m_savedX = pos().x(); }
    inline float savedX() { return m_savedX; }
    void updatePixmaps();
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
private:
    QPixmap m_reflection;
    GraphicsScene *m_scene;
    float m_rotate, m_savedX;
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
    void setModel( FileSystemModel *fsModel );
    void setCenterIndex( const QModelIndex &index );
    void showCenterIndex( const QModelIndex &index );
    inline void animateCenterIndex( const QModelIndex &index ) { m_scrollBar->setValue(index.row()); }
    inline FileSystemModel *fsModel() { return m_fsModel; }
    inline QModelIndex indexOfItem(PixmapItem *item) { return m_fsModel->index(m_items.indexOf(item), 0, m_rootIndex); }
    inline bool isAnimating() { return bool(m_timeLine->state() == QTimeLine::Running); }
    
signals:
    void centerIndexChanged( const QModelIndex &centerIndex );
    
public slots:
    inline void setRootIndex( const QModelIndex &rootIndex ) { clear(); m_rootIndex = rootIndex; }

protected:
    void resizeEvent(QResizeEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void populate(const int &start, const int &end);
    void prepareAnimation();
    bool isValidRow( const int &row ) { return bool(row > -1 && row < m_items.count()); }
    void correctItemsPos(const int &leftStart, const int &rightStart);
    void showPrevious();
    void showNext();

private slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void reset();
    void clear();
    void animStep( const qreal &value );
    void updateItemsPos();
    void scrollBarMoved( const int &value );
    void continueIf();

private:
    GraphicsScene *m_scene;
    FileSystemModel *m_fsModel;
    QModelIndex m_centerIndex, m_prevCenter, m_rootIndex;
    int m_row, m_nextRow, m_newRow;
    float m_y, m_x, m_perception;
    bool m_wantsDrag;
    QList<PixmapItem *> m_items;
    QGraphicsItemAnimation *m_anim[2];
    QTimeLine *m_timeLine;
    QGraphicsItem *m_pressed;
    QGraphicsSimpleTextItem *m_textItem;
    RootItem *m_rootItem;
    QGraphicsProxyWidget *m_gfxProxy;
    ScrollBar *m_scrollBar;
    QPointF m_pressPos;
};

}

#endif // PREVIEW_H
