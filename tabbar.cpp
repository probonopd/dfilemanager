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


#include "tabbar.h"
#include "config.h"
#include "mainwindow.h"
#include "operations.h"

#include <QTextLayout>
#include <qmath.h>

using namespace DFM;

WinButton::WinButton(Type t, QWidget *parent) : QWidget(parent), m_hasPress(false), m_type(t)
{
    switch ( t )
    {
    case Min:
        connect(this, SIGNAL(clicked()), MainWindow::currentWindow(), SLOT(showMinimized()));
        break;
    case Max:
        connect(this, SIGNAL(clicked()), this, SLOT(toggleMax()));
        break;
    case Close:
        connect(this, SIGNAL(clicked()), MainWindow::currentWindow(), SLOT(close()));
        break;
    default:break;
    }
    setFixedSize(16, 16);
    setAttribute(Qt::WA_Hover);
}

void
WinButton::mousePressEvent(QMouseEvent *e)
{
    m_hasPress = true;
    update();
    e->accept();
}

void
WinButton::mouseReleaseEvent(QMouseEvent *e)
{
    if ( m_hasPress && rect().contains(e->pos()) )
        emit clicked();
    m_hasPress = false;
    e->accept();
}

//static uint fcolors[3] = {0xFFBF0303, 0xFFF3C300, 0xFF00892B};
//static uint fcolors[3] = { 0xFFD86F6B, 0xFFD8CA6B, 0xFF76D86B };
//static uint fcolors[3] = { 0xFFBF2929, 0xFF29BF29, 0xFFBFBF29 };

static uint fcolors[3] = { 0xFFE55E4F, 0xFFF8C76D, 0xFFA1D886 };

void
WinButton::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.translate(0.5f, 0.5f);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r(rect().adjusted(3, 3, -4, -4));
    QColor fg(isActiveWindow() ? QColor(fcolors[m_type]) : QColor(127, 127, 127, 255));
    QColor mid(Operations::colorMid(fg, Qt::black, 12, 10));
    QRadialGradient rg(r.center()+QPointF(0, (underMouse()&&!m_hasPress)?-1:1), (float)r.width()*0.7f);
    rg.setColorAt(0.0f, fg);
    rg.setColorAt(1.0f, mid);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255,  127));
    p.drawEllipse(r.translated(0.0f, 1.0f));

    p.setPen(Operations::colorMid(fg, Qt::black));
    p.setBrush(rg);
    p.drawEllipse(r);
    p.end();
}

void
WinButton::toggleMax()
{
    if ( MainWindow::currentWindow()->isMaximized() )
        MainWindow::currentWindow()->showNormal();
    else
        MainWindow::currentWindow()->showMaximized();
}

FooBar::FooBar(QWidget *parent)
    : QWidget(parent)
    , m_tabBar(0L)
    , m_topMargin(8)
    , m_toolBar(MainWindow::currentWindow()->toolBar())
    , m_hasPress(false)
    , m_pressPos(QPoint())
{
    MainWindow::currentWindow()->installEventFilter(this);
    m_toolBar->installEventFilter(this);
    m_toolBar->setAttribute(Qt::WA_NoSystemBackground);
    m_toolBar->setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground);
}

int
FooBar::headHeight()
{
    int h = 0;
    if ( FooBar *foo = MainWindow::currentWindow()->findChild<FooBar *>() )
        h += foo->height();
    if ( QToolBar *bar = MainWindow::currentWindow()->toolBar() )
        if ( bar->isVisible() )
            h += bar->height();
    return h;
}

void
FooBar::setTabBar(TabBar *tabBar)
{
    m_tabBar = tabBar;
    QTimer::singleShot(200, this, SLOT(correctTabBarHeight()));
}

void
FooBar::correctTabBarHeight()
{
    m_tabBar->setFixedHeight(m_tabBar->tabSizeHint(m_tabBar->currentIndex()).height());
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addSpacing(4);
    layout->addWidget(new WinButton(WinButton::Close, this));
    layout->addSpacing(3);
    layout->addWidget(new WinButton(WinButton::Min, this));
    layout->addSpacing(3);
    layout->addWidget(new WinButton(WinButton::Max, this));
    layout->addSpacing(4);

    QHBoxLayout *tl = new QHBoxLayout();
    tl->setContentsMargins(0, m_topMargin, 0, 0);
    tl->setSpacing(0);
    tl->addWidget(m_tabBar);

    QFont font = m_tabBar->font();
    font.setPointSize(8);
    m_tabBar->setFont(font);

    layout->addLayout(tl);
    setLayout(layout);
    setContentsMargins(0, 0, 0, 0);
    setFixedHeight(m_tabBar->height()+m_topMargin);
    MainWindow::currentWindow()->setContentsMargins(0, height(), 0, 0);
}

QPainterPath
FooBar::tab(const QRect &r, int round, TabShape shape)
{
    int x = r.x(), y = r.y(), w = x+(r.width()-1), h = y+(r.height()-1);
    QPainterPath path;
    if ( shape == Standard )
    {
        path.moveTo(x, h);
        path.quadTo(x+(round), h, x+round, h-round);
        path.lineTo(x+round, y+round);
        path.quadTo(x+round, y, x+(round*2), y);
        path.lineTo(w-(round*2), y);
        path.quadTo(w-round, y, w-round, y+round);
        path.lineTo(w-round, h-round);
        path.quadTo(w-round, h, w, h);
    }
    else if ( shape == Chrome )
    {
        int half = h/2, hf = round/2;
        path.moveTo(x, h);
        path.quadTo(x+hf, h, x+round, half);
        path.quadTo(x+round+hf, y, x+(round*2), y);
        path.lineTo(w-(round*2), y);
        path.quadTo(w-(round+hf), y, w-round, half);
        path.quadTo(w-hf, h, w, h);
    }
    return path;
}

QRegion
FooBar::shape()
{
    int w = MainWindow::currentWindow()->width(), h = MainWindow::currentWindow()->height();
    QRegion mask = QRegion(0, 2, w, h-4);
    mask += QRegion(2, 0, w-4, h);
    mask += QRegion(1, 1, w-2, h-2);
    return mask;
}

QLinearGradient
FooBar::headGrad()
{
    QWidget *w = MainWindow::currentWindow();
    QPalette pal = w->palette();
    QColor bg(pal.color(w->backgroundRole()));
    QColor fg(pal.color(w->foregroundRole()));
//    QColor mid(Operations::colorMid(bg ,fg, 10, 1));
    QColor topMid(Operations::colorMid(Qt::white, bg));
    QColor bottomMid(Operations::colorMid(Qt::black, bg, 1, 3));
    QLinearGradient lg(0, 0, 0, headHeight());
    lg.setColorAt(0.0f, topMid);
    lg.setColorAt(1.0f, bottomMid);
    return lg;
}

void
FooBar::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.setBrushOrigin(rect().topLeft());
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.5f, 0.5f);

    QColor fg(parentWidget()->palette().color(parentWidget()->foregroundRole()));
    QColor bg(parentWidget()->palette().color(parentWidget()->backgroundRole()));

    QLinearGradient lg(0, 0, 0, height());
    lg.setColorAt(0.0f, bg/*Operations::colorMid(bg, fg, 8, 1)*/);
    lg.setColorAt(1.0f, Operations::colorMid(bg, fg, 2, 1));

    p.fillRect(rect(), lg);

    p.setPen(fg);
    p.drawLine(0, height()-2, width(), height()-2);
    p.setPen(QColor(255, 255, 255, bg.value()));
    p.drawLine(0, height()-1, width(), height()-1);

    QPainterPath path;
    int r = 5;
    path.moveTo(0, r);
    path.quadTo(0, 0, r, 0);
    path.lineTo(width()-r, 0);
    path.quadTo(width(), 0, width(), r);
    path.moveTo(0, r);
    path.closeSubpath();

    QLinearGradient pg(0, 0, 0, r);
    pg.setColorAt(0.0f, QColor(255, 255, 255, bg.value()));
    pg.setColorAt(1.0f, Qt::transparent);

    p.setPen(QPen(pg, 1.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(path);

    p.end();
}

bool
FooBar::eventFilter(QObject *o, QEvent *e)
{
    if ( o == MainWindow::currentWindow()
         && e->type() == QEvent::Resize )
    {
        resize(MainWindow::currentWindow()->width(), height());
        MainWindow::currentWindow()->setMask(shape());
        return false;
    }
    if ( o == m_toolBar && e->type() == QEvent::Paint )
    {
        QPainter p(m_toolBar);
        p.setBrushOrigin(0, -m_tabBar->height());
        p.fillRect(m_toolBar->rect(), headGrad());
        p.end();
        return true;
    }
    return QWidget::eventFilter(o, e);
}

void
FooBar::mousePressEvent(QMouseEvent *e)
{
    m_hasPress = true;
    m_pressPos = e->globalPos();
    e->accept();
}

void
FooBar::mouseReleaseEvent(QMouseEvent *e)
{
    m_hasPress = false;
    m_pressPos = QPoint();
    e->accept();
}

void
FooBar::mouseMoveEvent(QMouseEvent *e)
{
    if ( m_hasPress )
    {
        MainWindow::currentWindow()->move(MainWindow::currentWindow()->pos()+(e->globalPos()-m_pressPos));
        m_pressPos = e->globalPos();
    }
}

static QImage closer;

static QImage closeImg( QColor color )
{
    QImage img(QSize(16, 16), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.5f, 0.5f);
    p.setPen(QPen(color, 2.0f));
    int a = 4;
    p.setClipRect(img.rect().adjusted(0, a, 0, -(a+2)));
    p.drawLine(img.rect().topLeft(), img.rect().bottomRight());
    p.drawLine(img.rect().topRight(), img.rect().bottomLeft());
    p.end();
    return img;
}

void
TabCloser::paintEvent(QPaintEvent *)
{
    if ( closer.isNull() )
    {
        QColor fg(palette().color(foregroundRole()));
        QColor bg(palette().color(backgroundRole()));
        int y = bg.value()>fg.value()?1:-1;
        closer = QImage(QSize(16, 16), QImage::Format_ARGB32);
        closer.fill(Qt::transparent);
        QPainter p(&closer);
        p.drawImage(closer.rect().translated(0, y), closeImg(y==1?QColor(255,255,255,bg.value()):QColor(0,0,0,bg.value())));
        p.drawImage(closer.rect(), closeImg(fg));
        p.end();
    }

    QPainter pt(this);
    pt.drawImage(rect(), closer);
    pt.end();
}

TabBar::TabBar(QWidget *parent) : QTabBar(parent)
{
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setDocumentMode(true);
    if ( !Configuration::config.behaviour.gayWindow )
        setTabsClosable(true);
    else
        setAttribute(Qt::WA_NoSystemBackground);
    setMovable(true);
    setDrawBase(true);
    setExpanding(false);
    setElideMode(Qt::ElideRight);
}

void
TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()))
    {
        emit newTabRequest();
        event->accept();
        return;
    }
    QTabBar::mouseDoubleClickEvent(event);
}

void
TabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && tabAt(event->pos()) > -1)
    {
        emit tabCloseRequested(tabAt(event->pos()));
        event->accept();
        return;
    }
    QTabBar::mouseReleaseEvent(event);
}

void
TabBar::tabCloseRequest()
{
    TabCloser *tc = static_cast<TabCloser *>(sender());
    int index = tabAt(tc->geometry().center());
    emit tabCloseRequested(index);
}

void
TabBar::drawTab(QPainter *p, int index)
{
    QColor fg(parentWidget()->palette().color(foregroundRole()));
    QColor bg(parentWidget()->palette().color(backgroundRole()));

    QRect r(tabRect(index));
    QString s(tabText(index));
    if ( !r.isValid() )
        return;

    QLinearGradient it(r.topLeft(), r.bottomLeft());
    it.setColorAt(0.0f, bg);
    it.setColorAt(1.0f, Operations::colorMid(bg, fg, 3, 1));
    p->setBrush(Qt::NoBrush);


    FooBar::TabShape tabShape = (FooBar::TabShape)Configuration::config.behaviour.tabShape;
    int rndNess = Configuration::config.behaviour.tabRoundness;

    QRect shape(r);
    shape.setBottom(shape.bottom()-1);
//    shape.setTop(shape.top()+3);
    int overlap = qCeil((float)rndNess*1.5f);
    if ( shape.left() > overlap )
        shape.setLeft(shape.left()-overlap);
    if ( shape.right() < rect().width()-overlap )
        shape.setRight(shape.right()+overlap);

    p->setPen(fg);

    p->drawPath(FooBar::tab(shape, rndNess, tabShape));
    if ( index == currentIndex() )
        p->setBrush(FooBar::headGrad());
    else
        p->setBrush(it);

    p->setPen(QColor(255, 255, 255, bg.value()));
    p->drawPath(FooBar::tab(shape.adjusted(1, 1, -1, 1), rndNess, tabShape));

    if ( index == currentIndex() )
    {
        p->setPen(QColor(255, 255, 255, bg.value()));
        p->drawLine(rect().bottomLeft(), rect().bottomRight());
        p->setPen(Qt::NoPen);
        p->drawPath(FooBar::tab(shape.adjusted(2, 2, -2, 2), rndNess, tabShape));
    }

    tabIcon(index).paint(p, QRect(r.x() == rect().x() ? overlap : r.x(), r.y(), 16, r.bottom()-3));
    QFont f(font());
    f.setBold(index == currentIndex());
    int l = QFontMetrics(f).width(s);
    p->setFont(f);

    r.setRight(tabButton(index, RightSide)->geometry().x());
    r.setLeft(r.x() == rect().x() ? r.left()+20+overlap : r.left()+20);
    int y = bg.value() > fg.value() ? 1 : -1;
    QColor emboss(Operations::colorMid(bg, y==1 ? Qt::white : Qt::black, 2, 1));
    p->setPen(emboss);
    p->drawText(r.translated(0, y), /*l > r.width() ?*/ Qt::AlignLeft|Qt::AlignVCenter /*: Qt::AlignCenter*/, s);

    p->setPen(fg);
    p->drawText(r, /*l > r.width() ?*/ Qt::AlignLeft|Qt::AlignVCenter /*: Qt::AlignCenter*/, s);
}

void
TabBar::paintEvent(QPaintEvent *event)
{
    if ( !Configuration::config.behaviour.gayWindow )
    {
        QTabBar::paintEvent(event);
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(0.5f, 0.5f);

    for ( int i = 0; i < currentIndex(); ++i )
        drawTab(&p, i);
    for ( int i = count(); i > currentIndex(); --i )
        drawTab(&p, i);
    p.setPen(parentWidget()->palette().color(parentWidget()->foregroundRole()));
    p.drawLine(0, height()-2, width(), height()-2);
    drawTab(&p, currentIndex());
    p.end();
}

QSize
TabBar::tabSizeHint(int index) const
{
    if ( Configuration::config.behaviour.gayWindow )
        return QSize(qMin(150,rect().width()/count()), Configuration::config.behaviour.tabHeight/*QTabBar::tabSizeHint(index).height()+3*/);
    else
        return QSize(qMin(150,rect().width()/count()), QTabBar::tabSizeHint(index).height());
}

void
TabBar::tabInserted(int index)
{
    QTabBar::tabInserted(index);
    if ( Configuration::config.behaviour.gayWindow )
    {
        TabCloser *tc = new TabCloser();
        connect(tc, SIGNAL(clicked()), this, SLOT(tabCloseRequest()));
        setTabButton(index, RightSide, tc);
    }
}
