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

#include <QImageReader>
#include <QWheelEvent>
#include <QFileInfo>
#include <QStyleOptionGraphicsItem>
#include <QCoreApplication>
#include <QBitmap>
#include <QElapsedTimer>
#include <QApplication>
#include <qmath.h>
#include <QTransform>
#include <QRegion>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsItemAnimation>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsProxyWidget>
#include <QGraphicsEffect>
#include <QItemSelectionModel>
#include <QHash>
#include <QMouseEvent>
#include <QGLWidget>
#include <QList>
#include <QTimeLine>

#include "operations.h"
#include "flow.h"
#include "fsmodel.h"
#include "dataloader.h"

using namespace DFM;

#define ANGLE 66.0f
#define SCALE 0.80f
#define PERSPECTIVE 0.6f
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

    if (groove.isEmpty() || slider.isEmpty())
        return;
    // we need to paint to pixmaps first to get antialiasing...

    if (m_groove.isNull())
    {
        //groove
        const int h = palette().color(QPalette::Highlight).hue();
        QColor c; c.setHsv(h, 128, 32, 192);
        QPixmap grvPix(groove.size());
        grvPix.fill(Qt::transparent);
        const int grvR = qCeil(grvPix.height()/2.0f);
        QPainter grvPt(&grvPix);

        grvPt.setRenderHint(QPainter::Antialiasing);
        grvPt.setPen(Qt::NoPen);
        grvPt.setBrush(c);
        grvPt.drawRoundedRect(grvPix.rect(), grvR, grvR);
        grvPt.end();
        m_groove = grvPix;

        //slider
        c.setHsv(h, 32, 222);
        for (int i = 0; i < 2; ++i)
        {
            c.setAlpha(i?128:64);
            QPixmap sldrPix(slider.size());
            sldrPix.fill(Qt::transparent);
            const int sldrR = qCeil(sldrPix.height()/2.0f);
            QPainter sldrPt(&sldrPix);
            sldrPt.setRenderHint(QPainter::Antialiasing);
            sldrPt.setPen(Qt::NoPen);
            sldrPt.setBrush(c);
            sldrPt.drawRoundedRect(sldrPix.rect(), sldrR, sldrR);
            sldrPt.end();
            m_slider[i] = sldrPix;
        }
    }
    p.drawTiledPixmap(groove, m_groove);
    p.drawTiledPixmap(slider, m_slider[underMouse()]);
    p.end();
}

static QColor bg;

void
GraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    //    QGraphicsScene::drawBackground(painter, rect);
    if (!bg.isValid())
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

PixmapItem::PixmapItem(GraphicsScene *scene, QGraphicsItem *parent, QPixmap *pix)
    : QGraphicsItem(parent)
    , m_scene(scene)
    , m_isDirty(true)
{
    for (int i=0; i<2; ++i)
        m_pix[i]=pix[i];
    m_preView = m_scene->preView();
    setY(m_preView->m_y);
    setTransformOriginPoint(boundingRect().center());
}

PixmapItem::~PixmapItem()
{
    if (m_preView->m_items.contains(this))
        m_preView->m_items.removeOne(this);
    if (m_preView->m_dataLoader->hasInQueue(this))
        m_preView->m_dataLoader->removeFromQueue(this);
}

void
PixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (m_isDirty)
    {
        m_preView->m_dataLoader->updateItem(this);
        m_isDirty = false;
    }
    if (painter->transform().isScaling())
        painter->setRenderHints(QPainter::SmoothPixmapTransform);
    painter->drawTiledPixmap(QRect(0, SIZE, SIZE, SIZE), m_pix[1]);
    painter->drawTiledPixmap(QRect(0, 1, SIZE, SIZE), m_pix[0]);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, false);
}

QPainterPath
PixmapItem::shape() const
{
    return m_shape;
}

QModelIndex
PixmapItem::index()
{
    return m_preView->indexOfItem(this);
}

void
PixmapItem::updateShape()
{
    if (m_pix[0].isNull())
        return;
    QRegion r = m_pix[0].mask();
    QPainterPath p;
    p.addRegion(r);
    m_shape = p;
    m_preView->viewport()->update();
}

void
PixmapItem::transform(const float angle, const Qt::Axis axis, const float xscale, const float yscale)
{
    QTransform t;
    t.translate(SIZE/2.0f, SIZE*PERSPECTIVE);
    t.rotate(angle, axis);
    t.translate(-SIZE/2.0f, -SIZE*PERSPECTIVE);
    t.translate(SIZE/2.0f, SIZE/2.0f);
    t.scale(1.0f/*+(1.0f-xscale)/2*/, yscale);
    t.translate(-SIZE/2.0f, -SIZE/2.0f);
    setTransform(t);
    m_rotate = angle;
}

Flow::Flow(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new GraphicsScene(rect(), this))
    , m_model(0)
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
    , m_hasZUpdate(false)
    , m_xpos(0.0f)
    , m_dataLoader(new FlowDataLoader(this))
{
    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(false);
    glf.setSwapInterval(0);
    glf.setStencil(true);
    glf.setAccum(true);
    glf.setDirectRendering(true);
    glf.setDoubleBuffer(true);
    QGLFormat::setDefaultFormat(glf);
    setMaximumHeight(SIZE*2.0f);
    QGLWidget *glWidget = new QGLWidget(glf, this);
    connect(qApp, SIGNAL(aboutToQuit()), glWidget, SLOT(deleteLater()));
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
    m_scrollBar->setAttribute(Qt::WA_NoSystemBackground);
    connect(m_scrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollBarMoved(int)));
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    for (int i = 0; i < 2; ++i)
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

    QImage img[2];
    img[0] = FS::FileIconProvider::typeIcon(FS::FileIconProvider::File).pixmap(SIZE).toImage();
    img[1] = FS::FileIconProvider::typeIcon(FS::FileIconProvider::Folder).pixmap(SIZE).toImage();
    for (int d=0; d<2; ++d) //isDir
    {
        m_defaultPix[d][0] = QPixmap::fromImage(Ops::flowImg(img[d]));
        m_defaultPix[d][1] = QPixmap::fromImage(Ops::reflection(img[d]));
    }
}

Flow::~Flow()
{
    m_dataLoader->discontinue();
    m_dataLoader->wait();
    qDeleteAll(m_items);
    m_items.clear();
}

QModelIndex
Flow::indexOfItem(PixmapItem *item)
{
    if (m_items.contains(item))
        return m_model->index(m_items.indexOf(item), 0, m_rootIndex);
    return QModelIndex();
}

void
Flow::showNext()
{
    m_nextRow = m_row+1;
    prepareAnimation();
}

void
Flow::showPrevious()
{
    m_nextRow = m_row-1;
    prepareAnimation();
}

void
Flow::prepareAnimation()
{
    if (!isValidRow(m_newRow))
        return;

    int i = m_items.count();
    while (--i > -1)
        m_items.at(i)->saveX();
#define CENTER QPoint(m_x-SIZE/2.0f, m_y)
#define LEFT QPointF((m_x-SIZE)-space, m_y)
#define RIGHT QPointF(m_x+space, m_y)
    m_anim[New]->setItem(m_items.at(validate(m_nextRow)));
    m_anim[New]->setPosAt(1, CENTER);
    m_anim[Prev]->setItem(m_items.at(validate(m_row)));
    m_anim[Prev]->setPosAt(1, m_nextRow > m_row ? LEFT : RIGHT);
#undef CENTER
#undef RIGHT
#undef LEFT
    m_hasZUpdate = false;
    m_timeLine->setDuration(qMax(1.0f,250.0f/qAbs(m_row-m_newRow)));
    m_timeLine->start();
}

void
Flow::animStep(const qreal value)
{
    if (m_items.isEmpty())
        return;

    const float f = SCALE+(value*(1.0f-SCALE)), s = space*value;
    const bool goingUp = m_nextRow > m_row;

    float rotate = ANGLE * value;
    m_items.at(m_row)->transform(goingUp ? -rotate : rotate, Qt::YAxis, SCALE/f, SCALE/f);

    rotate = ANGLE-rotate;
    m_items.at(m_nextRow)->transform(goingUp ? rotate : -rotate, Qt::YAxis, f, f);

#define UP m_items.at(i)->savedX()-s
#define DOWN m_items.at(i)->savedX()+s

    int i = m_items.count();
    while (--i > -1)
    {
        if (i != m_row && i != m_nextRow)
            m_items.at(i)->setX(goingUp ? UP : DOWN);
        if (!m_hasZUpdate)
            m_items.at(i)->setZValue(i>=m_nextRow ? m_items.at(qMin(m_items.count()-1, i+1))->zValue()+1 : m_items.at(qMax(0, i-1))->zValue()-1);
    }

    m_hasZUpdate = true;

    if (value == 1)
    {
        setCenterIndex(m_model->index(m_nextRow, 0, m_rootIndex));
        if (m_newRow == m_row)
        {
            m_newRow = -1;
            m_nextRow = -1;  
            emit centerIndexChanged(m_centerIndex);
        }
    }
}

void
Flow::continueIf()
{
    if (m_newRow > m_row)
        showNext();
    else if (m_newRow < m_row)
        showPrevious();
}

void
Flow::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        if (event->delta() > 0)
            m_perception += 1;
        else
            m_perception -= 1;
        const float &y = m_y+SIZE;
        m_rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(m_perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    }
    else if (event->modifiers() & Qt::MetaModifier)
        m_rootItem->setScale(m_rootItem->scale()+((float)event->delta()*0.001f));
    else
        m_scrollBar->setValue(m_scrollBar->value()+(event->delta()>0?-1:1));
//        QCoreApplication::sendEvent(m_scrollBar, event);
}

void
Flow::enterEvent(QEvent *e)
{
    QGraphicsView::enterEvent(e);
    if (QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();
}

void
Flow::setModel(FS::Model *model)
{
    m_model = model;
    connect(m_model, SIGNAL(dataChanged(const QModelIndex & , const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex & , const QModelIndex &)));
    connect(m_model, SIGNAL(layoutAboutToBeChanged()),this, SLOT(clear()));
    connect(m_model, SIGNAL(layoutChanged()),this, SLOT(reset()));
    connect(m_model, SIGNAL(modelAboutToBeReset()), this, SLOT(clear()));
    connect(m_model, SIGNAL(modelReset()),this, SLOT(reset()));
    connect(m_model, SIGNAL(finishedWorking()),this, SLOT(reset()));
    connect(m_model, SIGNAL(rowsInserted(const QModelIndex & , int , int)), this, SLOT(rowsInserted(const QModelIndex & , int , int)));
    connect(m_model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex & , int , int)), this, SLOT(rowsRemoved(const QModelIndex & , int , int)));
}

void
Flow::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    const bool isFlowing = isVisible();
    const int start = topLeft.row(), end = bottomRight.row();

    if (m_items.count() > end)
        for (int i = start; i <= end; ++i)
        {
            PixmapItem *item = m_items.at(i);
            if (m_dataLoader->hasInQueue(item))
                m_dataLoader->removeFromQueue(item);
            if (isFlowing)
                m_dataLoader->updateItem(item);
            else
                item->m_isDirty=true;
        }
}

void
Flow::setCenterIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (index.row())
    {
        m_savedRow = index.row();
        m_savedCenter = index;

    }
    else if (m_items.count() <= 1)
    {
        m_savedRow = 0;
        m_nextRow = 0;
        m_row = 0;
    }
    m_centerUrl = m_model->url(index);
    m_prevCenter = m_centerIndex;
    m_centerIndex = index;
    m_nextRow = m_row;
    m_row = qMin(index.row(), m_items.count()-1);
    m_textItem->setText(index.data().toString());
    m_textItem->setZValue(m_items.count()+2);
    m_gfxProxy->setZValue(m_items.count()+2);
    m_textItem->setPos(m_x-m_textItem->boundingRect().width()/2.0f, rect().bottom()-(bMargin+m_scrollBar->height()+m_textItem->boundingRect().height()));
}

void
Flow::reset()
{
    clear();
    if (m_model && m_model->rowCount(m_rootIndex))
    {
        populate(0, m_model->rowCount(m_rootIndex)-1);
        m_scrollBar->setRange(0, m_items.count()-1);
        m_scrollBar->setValue(qBound(0, m_savedRow, m_items.count()-1));
    }
}

void
Flow::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateScene();
}

void
Flow::showEvent(QShowEvent *event)
{
    QGraphicsView::showEvent(event);
    updateScene();
}

void
Flow::updateScene()
{
    if (!m_textItem)
        return;

    QModelIndex center = m_model->index(m_centerUrl);
    if (!center.isValid())
        center = m_model->index(0, 0, m_rootIndex);

    if (m_textItem->text().isEmpty())
        m_textItem->setText(center.data().toString());
    m_scene->setSceneRect(QRect(QPoint(0, 0), size()));
    const float bottom = bMargin+m_scrollBar->height()+m_textItem->boundingRect().height();
    m_y = (height()/2.0f-SIZE/2.0f)-bottom;
    m_x = width()/2.0f;
    m_rootItem->update();
    updateItemsPos();
    m_scrollBar->resize(width()*0.66f, m_scrollBar->height());
    const float y = m_y+SIZE;
    const float scale = qMin<float>(1.0f, ((float)height()/SIZE)*0.8);
    m_textItem->setPos(qMax<float>(0.0f, m_x-m_textItem->boundingRect().width()/2.0f), rect().bottom()-(bMargin+m_scrollBar->height()+m_textItem->boundingRect().height()));
    m_textItem->setZValue(m_items.count()+2);
    m_gfxProxy->setPos(qRound(m_x-m_gfxProxy->boundingRect().width()/2.0f), qRound(rect().bottom()-(bMargin+m_scrollBar->height())));
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
Flow::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (!parent.isValid())
        return;

    if (m_items.isEmpty())
        return;

    m_timeLine->stop();

    for (int i = start; i <= end; ++i)
        if (i > 1 && i < m_items.size())
            delete m_items.takeAt(i);

    m_scrollBar->blockSignals(true);
    m_scrollBar->setRange(0, m_items.count()-1);
    m_scrollBar->setValue(validate(start-1));
    m_scrollBar->blockSignals(false);
    QModelIndex center = m_model->index(validate(start-1), 0, m_rootIndex);
    if (m_items.count() == 1)
        center = m_model->index(0, 0, parent);
    setCenterIndex(center);
    updateItemsPos();
}

void
Flow::setRootIndex(const QModelIndex &rootIndex)
{
    clear();
    m_rootIndex = rootIndex;
    m_rootUrl = m_model->url(rootIndex);
    m_savedRow = 0;
    m_savedCenter = QModelIndex();
    m_centerUrl = QUrl();
    reset();
}

void
Flow::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (m_model->isWorking())
        return;

    populate(start, end);

    if (!m_items.isEmpty())
        m_scrollBar->setRange(0, m_items.count()-1);
}

void
Flow::populate(const int start, const int end)
{
    for (int i = start; i <= end; i++)
    {
        bool isDir = m_model->isDir(m_model->index(i, 0, m_rootIndex));
        QPixmap pix[2];
        for (int i=0; i<2; ++i)
            pix[i]=m_defaultPix[isDir][i];
        m_items.insert(i, new PixmapItem(m_scene, m_rootItem, pix));
    }

    QModelIndex index;
    if (m_centerUrl.isValid())
        index = m_model->index(m_centerUrl);
    if (!index.isValid())
        index = m_model->index(validate(m_savedRow), 0, m_rootIndex);

    setCenterIndex(index);
    updateItemsPos();
    update();
    if (!m_items.isEmpty())
        setCenterIndex(m_model->index(0, 0, m_rootIndex));
}

void
Flow::updateItemsPos()
{
    if (m_items.isEmpty()
         || m_row == -1
         || m_row > m_model->rowCount(m_rootIndex)-1
         || !isVisible())
        return;

    m_timeLine->stop();
    PixmapItem *center = m_items.at(m_row);
    center->setZValue(m_items.count());
    center->setPos(m_x-SIZE/2.0f, m_y);
    center->resetTransform();

    if (m_items.count() > 1)
        correctItemsPos(m_row-1, m_row+1);
}

void
Flow::correctItemsPos(const int leftStart, const int rightStart)
{
    int z = m_items.count()-1;
    PixmapItem *p = 0;
    QList<PixmapItem *>::iterator i = m_items.begin()+rightStart;
//    int hideAfter = 100, current = 0;
    if (i < m_items.end())
    {
        p = *i;
        p->setPos(rect().center().x()+space, m_y);
        p->transform(ANGLE, Qt::YAxis, SCALE, SCALE);
        m_xpos = p->x();
        p->setZValue(z);
        while (++i < m_items.end()) //right side
        {
            --z;
            p = *i;
//            if (current > hideAfter)
//            {
//                p->setVisible(false);
//                continue;
//            }
//            else
//                p->setVisible(true);
            p->setPos(m_xpos+=space, m_y);
            p->transform(ANGLE, Qt::YAxis, SCALE, SCALE);
            p->setZValue(z);
//            ++current;
        }
    }

    m_xpos = 0.0f;
    z = m_items.count()-1;
    i = m_items.begin()+leftStart;
//    current = 0;
    if (i > m_items.begin()-1)
    {
        p = *i;
        p->setPos((rect().center().x()-SIZE)-space, m_y);
        p->transform(-ANGLE, Qt::YAxis, SCALE, SCALE);
        m_xpos = p->x();
        p->setZValue(z);

        while (--i > m_items.begin()-1) //left side
        {
            --z;
            p = *i;
//            if (current > hideAfter)
//            {
//                p->setVisible(false);
//                continue;
//            }
//            else
//                p->setVisible(true);
            p->setPos(m_xpos-=space, m_y);
            p->transform(-ANGLE, Qt::YAxis, SCALE, SCALE);
            p->setZValue(z);
//            ++current;
        }
    }
    p = 0;
    m_xpos = 0.0f;
}


void
Flow::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    if (itemAt(event->pos()))
        m_pressed = itemAt(event->pos());
    else
    {
        m_pressed = 0;
        m_wantsDrag = true;
        m_pressPos = event->pos();
    }
}

void
Flow::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if (itemAt(event->pos()) && m_pressed && itemAt(event->pos()) == m_pressed)
    {
        const QModelIndex &index = indexOfItem(static_cast<PixmapItem *>(m_pressed));
        if (index.isValid())
        {
            if (index == m_centerIndex)
            {
                const QFileInfo &file = m_model->fileInfo(index);
                if (file.isDir())
                    m_model->setUrl(QUrl::fromLocalFile(file.filePath()));
                else
                    Ops::openFile(file.filePath());
            }
            else
                m_scrollBar->setValue(index.row());
        }
    }
    m_wantsDrag = false;
}

void
Flow::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    if (!m_wantsDrag)
        return;
    if (event->pos().y() < m_pressPos.y())
        m_perception -= qAbs(event->pos().y() - m_pressPos.y())*0.1;
    else
        m_perception += qAbs(event->pos().y() - m_pressPos.y())*0.1;
    float y = m_y+SIZE;
    m_rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(m_perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    m_pressPos = event->pos();
}

void
Flow::showCenterIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    if (!isVisible()) //not visible... we silently update the index w/o animations
    {
        setCenterIndex(index);
        updateItemsPos();
        return;
    }

    m_newRow = index.row();

    if (m_newRow == m_row)
        return;

    if (qAbs(m_newRow-m_row) > 10)
    {
        int i = m_newRow > m_row ? -10 : +10;
        m_timeLine->stop();
        setCenterIndex(m_model->index(m_newRow+i, 0, m_rootIndex));
        updateItemsPos();
        showCenterIndex(m_model->index(m_newRow, 0, m_rootIndex));
        return;
    }
    if (m_timeLine->state() & QTimeLine::Running)
        return;
    if (m_newRow > m_row)
        showNext();
    else if (m_newRow < m_row)
        showPrevious();
}

void
Flow::clear()
{
    m_timeLine->stop();
    m_centerIndex = QModelIndex();
    m_prevCenter = QModelIndex();
    m_row = -1;
    m_nextRow = -1;
    m_newRow = -1;
    m_pressed = 0;
    m_savedRow = -1;
    m_savedCenter = QModelIndex();
    m_centerUrl = QUrl();
    m_savedRow = 0;
    qDeleteAll(m_items);
    m_textItem->setText(QString("--"));
    m_scrollBar->setValue(0);
    m_scrollBar->setRange(0, 0);
}

void
Flow::scrollBarMoved(const int value)
{
    if (m_items.count())
        showCenterIndex(m_model->index(qBound(0, value, m_items.count()), 0, m_rootIndex));
}

void
Flow::setSelectionModel(QItemSelectionModel *model)
{
    m_selectionModel = model;
}

void
Flow::animateCenterIndex(const QModelIndex &index)
{
    m_scrollBar->setValue(index.row());
}

bool
Flow::isAnimating()
{
    return bool(m_timeLine->state() == QTimeLine::Running);
}

//-----------------------------------------------------------------------------

FlowDataLoader::FlowDataLoader(QObject *parent)
    : DThread(parent)
    , m_preView(static_cast<Flow *>(parent))
{
    connect(this, SIGNAL(newData(PixmapItem*,QImage,QImage)), this, SLOT(setPixmaps(PixmapItem*,QImage,QImage)));
    start();
}

void
FlowDataLoader::updateItem(PixmapItem *item)
{
    const QImage &img = item->index().data(Qt::DecorationRole).value<QIcon>().pixmap(SIZE).toImage();
    if (!img.isNull() && !hasInQueue(item))
    {
        m_mtx.lock();
        m_queue << QPair<PixmapItem *, QImage>(item, img);
        m_mtx.unlock();
        setPause(false);
    }
}

void
FlowDataLoader::setPixmaps(PixmapItem *item, const QImage &img, const QImage &refl)
{
    if (m_preView->m_items.contains(item))
    {
        item->m_pix[0] = QPixmap::fromImage(img);
        item->m_pix[1] = QPixmap::fromImage(refl);
        item->updateShape();
    }
}

void
FlowDataLoader::genNewData(const QPair<PixmapItem *, QImage> &pair)
{
    PixmapItem *item = pair.first;
    const QImage &img = Ops::flowImg(pair.second);
    const QImage &refl = Ops::reflection(pair.second);
    emit newData(item, img, refl);
}

void
FlowDataLoader::run()
{
    while (!m_quit)
    {
        while (hasQueue())
            genNewData(dequeue());
        setPause(!m_quit);
        pause();
    }
}

bool
FlowDataLoader::hasQueue()
{
    QMutexLocker locker(&m_mtx);
    return !m_queue.isEmpty();
}

QPair<PixmapItem *, QImage>
FlowDataLoader::dequeue()
{
    QMutexLocker locker(&m_mtx);
    return m_queue.dequeue();
}

void
FlowDataLoader::discontinue()
{
    QMutexLocker locker(&m_mtx);
    m_queue.clear();
    DThread::discontinue();
}

bool
FlowDataLoader::hasInQueue(PixmapItem *item)
{
    QMutexLocker locker(&m_mtx);
    for (int i=0; i<m_queue.size(); ++i)
        if (m_queue.at(i).first == item)
            return true;
    return false;
}

void
FlowDataLoader::removeFromQueue(PixmapItem *item)
{
    QMutexLocker locker(&m_mtx);
    for (int i=0; i<m_queue.size(); ++i)
        if (m_queue.at(i).first == item)
        {
            m_queue.removeAt(i);
            return;
        }
}

