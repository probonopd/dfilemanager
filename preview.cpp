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


#include "preview.h"
#include "operations.h"
#include <QImageReader>
#include <QWheelEvent>
#include <QFileInfo>
#include <QStyleOptionGraphicsItem>
#include <QCoreApplication>
#include <QBitmap>
#include <QElapsedTimer>
#include <QApplication>
#include "operations.h"
#include <qmath.h>
#include <QTransform>
#include <QRegion>

using namespace DFM;

#define ANGLE 67.5f
#define SCALE 0.70f
#define PERSPECTIVE 0.66f
#define TESTPERF1 QElapsedTimer timer; timer.start()
#define TESTPERF2(_TEXT_) qDebug() << _TEXT_ << timer.elapsed() << "millisecs"

static float space = 48.0f, bMargin = 8;

void
ScrollBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    const QRect &groove = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);
    const QRect &slider = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this).adjusted(1, 1, -1, -1);

    // we need to paint to pixmaps first to get antialiasing...

    QPixmap grvPix(groove.size());
    grvPix.fill(Qt::transparent);
    const int grvR = qCeil(grvPix.height()/2.0f);
    QPainter grvPt(&grvPix);
    const int h = palette().color(QPalette::Highlight).hue();
    QColor c; c.setHsv(h, 127, 32, 192);
    grvPt.setRenderHint(QPainter::Antialiasing);
    grvPt.setPen(QColor(0,0,0,128));
    grvPt.setBrush(c);
    grvPt.drawRoundedRect(grvPix.rect(),grvR,grvR);
    grvPt.end();
    p.drawTiledPixmap(groove, grvPix);

    QPixmap sldrPix(slider.size());
    sldrPix.fill(Qt::transparent);
    const int sldrR = qCeil(sldrPix.height()/2.0f);
    QPainter sldrPt(&sldrPix);
    sldrPt.setRenderHint(QPainter::Antialiasing);
    sldrPt.setPen(Qt::NoPen);
    sldrPt.setBrush(Qt::white);
    sldrPt.drawRoundedRect(sldrPix.rect(),sldrR,sldrR);
    sldrPt.end();
    p.drawTiledPixmap(slider, sldrPix);

    p.end();
}

static QColor bg;

void
GraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    //    QGraphicsScene::drawBackground(painter, rect);
    if ( !bg.isValid() )
    {
        bg = Ops::colorMid(palette().color(QPalette::Highlight), Qt::black);
        bg.setHsv(bg.hue(), qMin(64, bg.saturation()), bg.value(), bg.alpha());
    }
    painter->fillRect(rect, bg);
}

void
GraphicsScene::drawForeground(QPainter *painter, const QRectF &rect)
{
//    QGraphicsScene::drawForeground(painter, rect);
    painter->fillRect(rect, m_fgBrush);
}

PixmapItem::PixmapItem(GraphicsScene *scene, QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , m_scene(scene)
    , m_isUpdateingPixmaps(false)
{
    m_preView = m_scene->preView();
    setY(m_preView->m_y);
    setTransformOriginPoint(boundingRect().center());
}

void
PixmapItem::updatePixmaps()
{
    const QString &file = m_preView->fsModel()->filePath(m_preView->indexOfItem(this));
    const QModelIndex &index = m_preView->fsModel()->index(file);
    m_pix[0] = qvariant_cast<QPixmap>(m_preView->fsModel()->data(index, FileSystemModel::FlowImg));
    m_pix[1] = qvariant_cast<QPixmap>(m_preView->fsModel()->data(index, FileSystemModel::FlowRefl));
    m_shape = QVariant::fromValue(m_preView->fsModel()->data(index, FileSystemModel::FlowShape)).value<QPainterPath>();
//    qDebug() << m_shape.isEmpty() << file;
    updateShape();
    m_preView->viewport()->update();
}

void
PixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if ( m_pix[1].isNull() )
        updatePixmaps();
#if 0
    if (option->state & QStyle::State_MouseOver) //just to have some hover effect...
    {
        painter->translate(SIZE/2.0f, SIZE/2.0f);
        painter->scale(1.1, 1.1);
        painter->translate(-SIZE/2.0f, -SIZE/2.0f);
    }
#endif
    painter->drawTiledPixmap(QRect(0, SIZE, SIZE, SIZE), m_pix[1]);
    if ( painter->transform().isScaling() )
        painter->setRenderHints(QPainter::SmoothPixmapTransform);
    painter->drawTiledPixmap(QRect(0, 1, SIZE, SIZE), m_pix[0]);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, false);
}

void
PixmapItem::updateShape()
{
    QRegion r = m_pix[0].mask();
    QPainterPath p;
    p.addRegion(r);
    m_shape = p;
}

void
PixmapItem::rotate(const float angle, const Qt::Axis axis)
{
    QTransform t;
    t.translate(SIZE/2.0f, SIZE*PERSPECTIVE);
    t.rotate(angle, axis);
    t.translate(-SIZE/2.0f, -SIZE*PERSPECTIVE);
    setTransform(t);
    m_rotate = angle;
}

PreView::PreView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new GraphicsScene(rect(), this))
    , m_fsModel(0)
    , m_row(-1)
    , m_nextRow(-1)
    , m_newRow(-1)
    , m_savedRow(-1)
    , m_pressed(0)
    , m_y(0.0f)
    , m_x(0.0f)
    , m_timeLine(new QTimeLine(250, this))
    , m_scrollBar(0)
    , m_rootItem(new RootItem(m_scene))
    , m_wantsDrag(false)
    , m_perception(0.0f)
    , m_pressPos(QPointF())
    , m_centerFile(QString())
    , m_hasZUpdate(false)
{
    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(false);
    glf.setSwapInterval(0);
    glf.setStencil(true);
    glf.setAccum(false);
    glf.setDirectRendering(true);
    glf.setDoubleBuffer(true);
    QGLFormat::setDefaultFormat(glf);
    setMaximumHeight(SIZE*2.0f);
    QGLWidget *glWidget = new QGLWidget(glf, this);
    connect( qApp, SIGNAL(aboutToQuit()), glWidget, SLOT(deleteLater()) );
    setViewport(glWidget);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    setCacheMode(QGraphicsView::CacheBackground);
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(m_scene);
    m_textItem = new QGraphicsSimpleTextItem();
    ScrollBar *scrollBar = new ScrollBar(Qt::Horizontal);
    scrollBar->setSingleStep(1);
    scrollBar->setPageStep(1);
    m_gfxProxy = m_scene->addWidget(scrollBar);
    m_scrollBar = static_cast<ScrollBar *>(m_gfxProxy->widget());
    connect( m_scrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollBarMoved(int)) );
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    for ( int i = 0; i < 2; ++i )
    {
        m_anim[i] = new QGraphicsItemAnimation(this);
        m_anim[i]->setTimeLine(m_timeLine);
    }
    m_timeLine->setEasingCurve(QEasingCurve::Linear);
    m_timeLine->setUpdateInterval(17); //17 ~ 60 fps
    connect(m_timeLine, SIGNAL(valueChanged(qreal)), this, SLOT(animStep(qreal)));
    connect(m_timeLine, SIGNAL(finished()), this, SLOT(continueIf()));

    m_textItem = new QGraphicsSimpleTextItem();
    m_scene->addItem(m_textItem);
    QFont f = font();
    f.setPointSize(16);
    f.setBold(true);
    m_textItem->setFont(f);
    m_textItem->setBrush(Qt::white);
    QGraphicsDropShadowEffect *dse = new QGraphicsDropShadowEffect(this);
    dse->setBlurRadius(4);
    dse->setOffset(0);
    dse->setColor(Qt::black);
    m_textItem->setGraphicsEffect(dse);
    setMouseTracking(false);
    setAttribute(Qt::WA_Hover, false);
    setCursor(Qt::ArrowCursor);
}

QModelIndex
PreView::indexOfItem(PixmapItem *item)
{
    if ( m_items.contains(item) )
        return m_fsModel->index(m_items.indexOf(item), 0, m_rootIndex);
    return QModelIndex();
}

void
PreView::showNext()
{
    m_nextRow = m_row+1;
    prepareAnimation();
}

void
PreView::showPrevious()
{
    m_nextRow = m_row-1;
    prepareAnimation();
}

void
PreView::prepareAnimation()
{
    if ( !isValidRow(m_newRow) )
        return;

    int i = m_items.count();
    while ( --i > -1 )
        m_items.at(i)->saveX();
#define CENTER QPoint(m_x-SIZE/2.0f, m_y)
#define LEFT QPointF((m_x-SIZE)-space, m_y)
#define RIGHT QPointF(m_x+space, m_y)
    m_anim[New]->setItem(m_items.at(m_nextRow));
    m_anim[New]->setPosAt(1, CENTER);

    m_anim[Prev]->setItem(m_items.at(m_row));
    m_anim[Prev]->setPosAt(1, m_nextRow > m_row ? LEFT : RIGHT);
#undef CENTER
#undef RIGHT
#undef LEFT

    m_hasZUpdate = false;

    m_timeLine->setDuration(qMax(1.0f,250.0f/qAbs(m_row-m_newRow)));
    m_timeLine->start();
}

void
PreView::animStep(const qreal value)
{
    if ( m_items.isEmpty() )
        return;

    const float f = SCALE+(value*(1.0f-SCALE)), s = space*value;
    const bool goingUp = m_nextRow > m_row;

    m_items.at(m_row)->setScale(SCALE/f);
    float rotate = ANGLE * value;
    m_items.at(m_row)->rotate(goingUp ? -rotate : rotate, Qt::YAxis);

    m_items.at(m_nextRow)->setScale(f);
    rotate = ANGLE-rotate;
    m_items.at(m_nextRow)->rotate(goingUp ? rotate : -rotate, Qt::YAxis);

    int i = m_items.count();
    while ( --i > -1 )
    {
        if ( i != m_row && i != m_nextRow )
            m_items.at(i)->setX( goingUp ? m_items.at(i)->savedX()-s : m_items.at(i)->savedX()+s );
        if ( !m_hasZUpdate )
            m_items.at(i)->setZValue( i>=m_nextRow ? m_items.at(qMin(m_items.count()-1, i+1))->zValue()+1 : m_items.at(qMax(0, i-1))->zValue()-1 );
    }

    m_hasZUpdate = true;

    if ( value == 1 )
    {
        setCenterIndex(m_fsModel->index(m_nextRow, 0, m_rootIndex));
        if ( m_newRow == m_row )
        {
            m_newRow = -1;
            m_nextRow = -1;  
            emit centerIndexChanged(m_centerIndex);
        }
    }
}

void
PreView::continueIf()
{
    if ( m_newRow > m_row )
        showNext();
    else if ( m_newRow < m_row )
        showPrevious();
}

void
PreView::wheelEvent(QWheelEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier )
    {
        if ( event->delta() > 0 )
            m_perception += 1;
        else
            m_perception -= 1;
        const float &y = m_y+SIZE;
        m_rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(m_perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    }
    else if ( event->modifiers() & Qt::MetaModifier )
        m_rootItem->setScale(m_rootItem->scale()+((float)event->delta()*0.001f));
    else
        m_scrollBar->setValue(m_scrollBar->value()+(event->delta()>0?-1:1));
//        QCoreApplication::sendEvent(m_scrollBar, event);
}

void
PreView::enterEvent(QEvent *e)
{
    QGraphicsView::enterEvent(e);
    if ( QApplication::overrideCursor() )
        QApplication::restoreOverrideCursor();
}

void
PreView::setModel(FileSystemModel *fsModel)
{
    m_fsModel = fsModel;
    connect(m_fsModel, SIGNAL(flowDataChanged(const QModelIndex & , const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex & , const QModelIndex &)));
    connect(m_fsModel, SIGNAL(layoutChanged()),this, SLOT(reset()));
    connect(m_fsModel, SIGNAL(modelReset()),this, SLOT(reset()));
    connect(m_fsModel, SIGNAL(rowsInserted(const QModelIndex & , int , int)), this, SLOT(rowsInserted(const QModelIndex & , int , int)));
    connect(m_fsModel, SIGNAL(rowsRemoved(const QModelIndex & , int , int)), this, SLOT(rowsRemoved(const QModelIndex & , int , int)));
}

void
PreView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    const int start = topLeft.row(), end = bottomRight.row();

    if ( m_items.count() > end )
    for ( int i = start; i <= end; ++i )
        m_items.at(i)->updatePixmaps();
}

void
PreView::setCenterIndex(const QModelIndex &index)
{
    if ( !index.isValid() || m_centerIndex == index )
        return;

    if ( index.row() )
    {
        m_savedRow = index.row();
        m_savedCenter = index;
        m_centerFile = m_fsModel->filePath(index);
    }
    m_prevCenter = m_centerIndex;
    m_centerIndex = index;
    m_nextRow = m_row;
    m_row = index.row();
    m_textItem->setText(index.data().toString());
    m_textItem->setZValue(m_items.count()+2);
    m_gfxProxy->setZValue(m_items.count()+2);
    m_textItem->setPos(m_x-m_textItem->boundingRect().width()/2.0f, rect().bottom()-(bMargin+m_scrollBar->height()+m_textItem->boundingRect().height()));
}

void
PreView::reset()
{
    clear();
    if ( !QFileInfo(m_rootPath).exists() )
        return;

    if ( m_rootIndex.isValid() && m_fsModel && m_fsModel->rowCount(m_rootIndex) )
    {
        populate(0, m_fsModel->rowCount(m_rootIndex)-1);
        m_scrollBar->setRange(0, m_fsModel->rowCount(m_rootIndex)-1);
        m_scrollBar->setValue(qBound(0, m_savedRow, m_items.count()));
    }
}

void
PreView::showEvent(QShowEvent *event)
{
    QGraphicsView::showEvent(event);
    const float bottom = bMargin+m_scrollBar->height()+m_textItem->boundingRect().height();
    m_y = (rect().size().height()/2.0f-SIZE/2.0f)-bottom;
    m_x = rect().size().width()/2.0f;
    if ( m_items.count() > m_row && m_row > -1 )
        m_items.at(m_row)->updatePixmaps();
    updateItemsPos();
}

void
PreView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    m_scene->setSceneRect(QRect(QPoint(0, 0), event->size()));
    const float bottom = bMargin+m_scrollBar->height()+m_textItem->boundingRect().height();
    m_y = (event->size().height()/2.0f-SIZE/2.0f)-bottom;
    m_x = event->size().width()/2.0f;
    m_rootItem->update();
    updateItemsPos();
    m_scrollBar->resize(event->size().width()*0.66f, m_scrollBar->height());
    const float y = m_y+SIZE;
    const float scale = qMin<float>(1.0f, ((float)rect().height()/SIZE)*0.8);
    m_textItem->setPos(m_x-m_textItem->boundingRect().width()/2.0f, rect().bottom()-(bMargin+m_scrollBar->height()+m_textItem->boundingRect().height()));
    m_textItem->setZValue(m_items.count()+2);
    m_gfxProxy->setPos(m_x-m_gfxProxy->boundingRect().width()/2.0f, rect().bottom()-(bMargin+m_scrollBar->height()));
    m_rootItem->setTransformOriginPoint(rect().center());
    m_rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(m_perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    m_rootItem->setScale(scale);

    QRadialGradient rg(QPoint(rect().width()/2.0f, rect().bottom()*0.75f), rect().width()/2.0f);
    rg.setColorAt(0, Qt::transparent);
    rg.setColorAt(0.5, Qt::transparent);
    rg.setColorAt(0.75, QColor(0,0,0,64));
    rg.setColorAt(1, QColor(0,0,0,192));
    m_scene->setForegroundBrush(rg);
}

void
PreView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if ( !QFileInfo(m_rootPath).exists() )
    {
        clear();
        return;
    }
    if ( m_rootIndex != parent || m_items.isEmpty() )
        return;

    m_timeLine->stop();

    for ( int i = start; i <= end; ++i )
        if ( m_items.count() > i )
            delete m_items.takeAt(i);

    m_scrollBar->blockSignals(true);
    m_scrollBar->setRange(0, m_items.count());
    m_scrollBar->setValue(qBound(0, start-1, m_items.count()));
    m_scrollBar->blockSignals(false);
    setCenterIndex(m_fsModel->index(qBound(0, start-1, m_items.count()), 0, m_rootIndex));
    updateItemsPos();
}

void
PreView::setRootIndex(const QModelIndex &rootIndex)
{
    clear();
    if ( !rootIndex.isValid() )
        return;
    m_rootIndex = rootIndex;
    m_rootPath = m_fsModel->filePath(rootIndex);
    m_savedRow = 0;
    m_savedCenter = QModelIndex();
    m_centerFile = QString();
}

void
PreView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if ( !parent.isValid() || m_rootIndex != parent )
        return;

    if ( !m_fsModel->hasIndex(start, 0, parent) || !m_fsModel->hasIndex(end, 0, parent) )
        return;

    populate(start, end);
}

void
PreView::populate(const int start, const int end)
{
    if ( !m_rootIndex.isValid() )
        return;

    for ( int i = start; i <= end; i++ )
    {
        const QModelIndex &index = m_fsModel->index(i, 0, m_rootIndex);
        if ( !index.isValid() )
            continue;
        m_items.insert(i, new PixmapItem(m_scene, m_rootItem));
    }

    QModelIndex index = m_fsModel->index(m_centerFile);
    if ( !index.isValid() )
        index = m_fsModel->index(qBound(0, m_savedRow, m_items.count()), 0, m_rootIndex);
    setCenterIndex(index);
    updateItemsPos();
    update();
}

void
PreView::updateItemsPos()
{
    if ( !m_items.count() || m_row == -1 || m_row > m_fsModel->rowCount(m_rootIndex)-1 || !isVisible() )
        return;

    m_timeLine->stop();
    PixmapItem *center = m_items.at(m_row);
    center->setZValue(m_items.count());
    center->setPos(m_x-SIZE/2.0f, m_y);
    center->setScale(1);
    center->resetTransform();

    correctItemsPos(m_row-1, m_row+1 );
}

static float xpos = 0.0f;

void
PreView::correctItemsPos(const int leftStart, const int rightStart)
{
    int z = m_items.count()-1;
    PixmapItem *p = 0;
    QList<PixmapItem *>::iterator i = m_items.begin()+rightStart;
    if ( i < m_items.end() )
    {
        p = *i;
        p->setPos( rect().center().x()+space, m_y );
        p->setScale(SCALE);
        p->rotate(ANGLE, Qt::YAxis);
        xpos = p->x();
        p->setZValue(z);
        while ( ++i < m_items.end() ) //right side
        {
            --z;
            p = *i;
            p->setPos(xpos+=space, m_y);
            p->setScale(SCALE);
            p->rotate(ANGLE, Qt::YAxis);
            p->setZValue(z);
        }
    }

    xpos = 0.0f;
    z = m_items.count()-1;
    i = m_items.begin()+leftStart;
    if ( i > m_items.begin()-1 )
    {
        p = *i;
        p->setPos( (rect().center().x()-SIZE)-space, m_y );
        p->setScale(SCALE);
        p->rotate(-ANGLE, Qt::YAxis);
        xpos = p->x();
        p->setZValue(z);

        while ( --i > m_items.begin()-1 ) //left side
        {
            --z;
            p = *i;
            p->setPos(xpos-=space, m_y);
            p->setScale(SCALE);
            p->rotate(-ANGLE, Qt::YAxis);
            p->setZValue(z);
        }
    }
    p = 0;
    xpos = 0.0f;
}


void
PreView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    if ( itemAt(event->pos()) )
        m_pressed = itemAt(event->pos());
    else
    {
        m_pressed = 0;
        m_wantsDrag = true;
        m_pressPos = event->pos();
    }
}

void
PreView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if ( itemAt(event->pos()) && m_pressed && itemAt(event->pos()) == m_pressed )
    {
        const QModelIndex &index = indexOfItem(static_cast<PixmapItem *>(m_pressed));
        if ( index.isValid() )
        {
            if ( index == m_centerIndex )
            {
                const QString &file = m_fsModel->filePath(index);
                if ( QFileInfo(file).isDir() )
                    m_fsModel->setRootPath(file);
                else
                    Ops::openFile(file);
            }
            else
                m_scrollBar->setValue(index.row());
        }
    }
    m_wantsDrag = false;
}

void
PreView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    if ( !m_wantsDrag )
        return;
    if ( event->pos().y() < m_pressPos.y() )
        m_perception -= qAbs(event->pos().y() - m_pressPos.y())*0.1;
    else
        m_perception += qAbs(event->pos().y() - m_pressPos.y())*0.1;
    float y = m_y+SIZE;
    m_rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(m_perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    m_pressPos = event->pos();
}

void
PreView::showCenterIndex(const QModelIndex &index)
{
    if ( !index.isValid() )
        return;
    if ( !isVisible() ) //not visible... we silently update the index w/o animations
    {
        setCenterIndex(index);
        updateItemsPos();
        return;
    }

    m_newRow = index.row();

    if ( m_newRow == m_row )
        return;

    if ( qAbs(m_newRow-m_row) > 10 )
    {
        int i = m_newRow > m_row ? -10 : +10;
        m_timeLine->stop();
        setCenterIndex(m_fsModel->index(m_newRow+i, 0, m_rootIndex));
        updateItemsPos();
        showCenterIndex(m_fsModel->index(m_newRow, 0, m_rootIndex));
        return;
    }
    if ( m_timeLine->state() & QTimeLine::Running )
        return;
    if ( m_newRow > m_row )
        showNext();
    else if ( m_newRow < m_row )
        showPrevious();
}

void
PreView::clear()
{
    m_timeLine->stop();
    m_centerIndex = QModelIndex();
    m_prevCenter = QModelIndex();
    m_row = -1;
    m_newRow = -1;
    m_pressed = 0;
    while ( !m_items.isEmpty() )
        delete m_items.takeFirst();
    m_textItem->setText(QString());
    m_scrollBar->setValue(0);
    m_scrollBar->setRange(0, 0);
}

void
PreView::scrollBarMoved(const int value)
{
    if ( m_items.count() )
        showCenterIndex(m_fsModel->index(qBound(0, value, m_items.count()), 0, m_rootIndex));
}
