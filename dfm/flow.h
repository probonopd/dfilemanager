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


#ifndef FLOW_H
#define FLOW_H

#include <QScrollBar>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QQueue>
#include "objects.h"

class QModelIndex;
class QPersistentModelIndex;
class QItemSelectionModel;
class QGraphicsItemAnimation;
class QTimeLine;
namespace DFM
{
namespace FS{class Model;}
#define SIZE 258.0f
#define RECT QRectF(0.0f, 0.0f, SIZE, SIZE)
class Flow;
class FlowDataLoader;

class ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    inline explicit ScrollBar(const Qt::Orientation o, QWidget *parent = 0) : QScrollBar(o, parent) {}
    inline void setRange(int min, int max) { clearCache(); QScrollBar::setRange(min, max); }
    inline void resizeEvent(QResizeEvent *e) { clearCache(); QScrollBar::resizeEvent(e); }
    inline void clearCache() { for (int i = 0; i < 2; ++i) m_slider[i] = QPixmap(); m_groove = QPixmap(); }
protected:
    void paintEvent(QPaintEvent *);
    QPixmap m_slider[2], m_groove;
};

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    inline explicit GraphicsScene(const QRectF &rect = QRectF(), QWidget *parent = 0) : QGraphicsScene(rect, parent), m_preview(qobject_cast<Flow *>(parent)){}
    ~GraphicsScene(){}
    inline Flow *preView() { return m_preview; }
    inline void setForegroundBrush(const QBrush &brush) { m_fgBrush = brush; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);
    Flow *m_preview;
    QBrush m_fgBrush;
};

class RootItem : public QGraphicsPixmapItem
{
public:
    inline explicit RootItem(QGraphicsScene *scene = 0) : QGraphicsPixmapItem() { scene->addItem(this); }
    ~RootItem(){}
    QRectF boundingRect() { return scene()->sceneRect(); }
};

/* PIXMAP
 * ITEM
 */

class PixmapItem : public QGraphicsItem
{
public:
    PixmapItem(GraphicsScene *scene = 0, QGraphicsItem *parent = 0, QPixmap *pix = 0);
    ~PixmapItem();
    void transform(const float angle, const Qt::Axis axis, const float xscale = 1.0f, const float yscale = 1.0f);
    QRectF boundingRect() const { return RECT; }
    inline void saveX() { m_savedX = pos().x(); }
    inline float savedX() { return m_savedX; }
    QPainterPath shape() const;
    QModelIndex index();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void updateShape();

private:
    QPixmap m_pix[2];
    GraphicsScene *m_scene;
    Flow *m_preView;
    float m_rotate, m_savedX;
    bool m_isDirty;
    QPainterPath m_shape;
    friend class Flow;
    friend class FlowDataLoader;
};

/* THE
 * VIEW
 */

class Flow : public QGraphicsView
{
    Q_OBJECT
public:
    enum Pos { Prev = 0, New = 1 };
    explicit Flow(QWidget *parent = 0);
    ~Flow();
    void setModel(FS::Model *model);
    void setSelectionModel(QItemSelectionModel *model);
    void setCenterIndex(const QModelIndex &index);
    void showCenterIndex(const QModelIndex &index);
    void animateCenterIndex(const QModelIndex &index);
    QModelIndex indexOfItem(PixmapItem *item);
    bool isAnimating();
    
signals:
    void centerIndexChanged(const QModelIndex &centerIndex);
    
public slots:
    void setRootIndex(const QModelIndex &rootIndex);

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
    inline bool isValidRow(const int row) { return bool(row > -1 && row < m_items.count()); }
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
    void animStep(const qreal value);
    void updateItemsPos();
    void scrollBarMoved(const int value);
    void continueIf();
    void updateScene();

private:
    GraphicsScene *m_scene;
    FS::Model *m_model;
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
    QUrl m_rootUrl, m_centerUrl;
    FlowDataLoader *m_dataLoader;
    QPixmap m_defaultPix[2][2];
    friend class PixmapItem;
    friend class FlowDataLoader;
};

class FlowDataLoader : public DThread
{
    Q_OBJECT
public:
    enum Type { ThemeIcon = 0, FileIcon = 1 };
    explicit FlowDataLoader(QObject *parent = 0);
    void updateItem(PixmapItem *item);

public slots:
    void discontinue();

signals:
    void newData(PixmapItem *item, const QImage &img, const QImage &refl);

protected:
    void run();
    void genNewData(const QPair<PixmapItem *, QImage> &pair);
    bool hasInQueue(PixmapItem *item);
    void removeFromQueue(PixmapItem *item);
    bool hasQueue();
    QPair<PixmapItem *, QImage> dequeue();

protected slots:
    void setPixmaps(PixmapItem *item, const QImage &img, const QImage &refl);

private:
    QQueue<QPair<PixmapItem *, QImage> > m_queue;
    Flow *m_preView;
    mutable QMutex m_mtx;
    friend class PixmapItem;
    friend class Flow;
};

}

#endif // FLOW_H
