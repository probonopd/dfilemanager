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

#include <QTimer>
#include <QStyle>
#include "dockwidget.h"
#include "mainwindow.h"
#include "titlewidget.h"
#include "config.h"

using namespace DFM;
using namespace Docks;

DockWidget::DockWidget(QWidget *parent, const QString &title, const Qt::WindowFlags &flags, const Pos &pos)
    : QDockWidget(title, parent, flags)
//    , m_margin(0)
//    , m_titleWidget(new TitleWidget(this, title, pos))
    , m_position(pos)
    , m_timer(new QTimer(this))
    , m_animStep(0)
    , m_dirIn(false)
    , m_isLocked(false)
{
//    if (qApp->styleSheet().isEmpty())
//        setStyle(new MyStyle(style()->objectName()));
//    m_margin = style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth);
    setAllowedAreas(m_position == Left ? Qt::LeftDockWidgetArea : m_position == Right ? Qt::RightDockWidgetArea : Qt::BottomDockWidgetArea);
    setFeatures(DockWidgetClosable | DockWidgetFloatable);
//    setTitleBarWidget(m_titleWidget);
//    setFocusPolicy(Qt::NoFocus);
//    APP->manageDock(this);
//    connect (m_timer, SIGNAL(timeout()), this, SLOT(animate()));
//    connect (m_titleWidget->floatButton(), SIGNAL(clicked()), this, SLOT(setFloat()));
//    connect (this, SIGNAL(topLevelChanged(bool)), this, SLOT(floatationChanged(bool)));
//    connect (this, SIGNAL(topLevelChanged(bool)), m_titleWidget,   SLOT(setIsFloating(bool)));
//    m_mainWindow->installEventFilter(this);
    QTimer::singleShot(0, this, SLOT(postConstructorJobs()));
    m_margins = contentsMargins();
    if (style()->objectName() != "oxygen")
        setContentsMargins(0, 0, 0, 0);
}

void
DockWidget::postConstructorJobs()
{
    setLocked(bool(Store::config.docks.lock & m_position));
}

void
DockWidget::setLocked(const bool locked)
{
    if (isFloating())
        return;
    if (locked)
    {
        Store::config.docks.lock |= m_position;
        QLabel *w = new QLabel(this);
        w->setContentsMargins(0, -w->fontMetrics().height(), 0, -w->fontMetrics().height());
        setTitleBarWidget(w);
        m_isLocked = true;
        if (style()->objectName() != "oxygen")
            setContentsMargins(0, 0, 0, 0);
    }
    else
    {
        Store::config.docks.lock &= ~m_position;
        if (QWidget *w = titleBarWidget())
            delete w;
        setTitleBarWidget(0);
        m_isLocked = false;
        if (style()->objectName() != "oxygen")
            setContentsMargins(m_margins);
    }
}

void
DockWidget::floatationChanged(const bool floating)
{
#if 0
    m_timer->stop();
    m_animStep = 1;
    if (floating)
    {
        setWindowFlags(Qt::Window); // | Qt::X11BypassWindowManagerHint
        show();
        if (m_position < Bottom)
            resize(width(), m_mainWindow->height()-32);
        else
            resize(m_mainWindow->width()-32, height());
        m_timer->start(10);
        m_mainWindow->raise();
    }
    else
        if (!mask().isEmpty())
            clearMask();
#endif
}

#if 0 //6px roundness
{
case Left:
    mask = QRegion(4, 0, w-4, h);
    mask += QRegion(0, 4, w, h-8);
    mask += QRegion(2, 1, w-2, h-2);
    mask += QRegion(1, 2, w-1, h-4);
    break;
case Right:
    mask = QRegion(0, 0, w-4, h);
    mask += QRegion(0, 4, w, h-8);
    mask += QRegion(0, 1, w-2, h-2);
    mask += QRegion(0, 2, w-1, h-4);
    break;
case Bottom:
    mask = QRegion(0, 0, w, h-4);
    mask += QRegion(4, 0, w-8, h);
    mask += QRegion(1, 0, w-2, h-2);
    mask += QRegion(2, 0, w-4, h-1);
    break;
}
#endif

QRegion
DockWidget::shape() const
{
#if 0
    int h = m_position < Bottom ? m_mainWindow->height()-32 : height(), w = width();
    QRegion mask;
    switch (m_position)
    {
    case Left:
        mask = QRegion(2, 0, w-2, h);
        mask += QRegion(0, 2, w, h-4);
        mask += QRegion(1, 1, w-1, h-2);
        break;
    case Right:
        mask = QRegion(0, 0, w-2, h);
        mask += QRegion(0, 2, w, h-4);
        mask += QRegion(0, 1, w-1, h-2);
        break;
    case Bottom:
        mask = QRegion(0, 0, w, h-2);
        mask += QRegion(2, 0, w-4, h);
        mask += QRegion(1, 0, w-2, h-1);
        break;
    }
    return mask;
#else
    return QRegion();
#endif
}

bool
DockWidget::eventFilter(QObject *object, QEvent *event)
{
#if 0
    if (m_mainWindow && isFloating() && object == m_mainWindow)
        if (event->type() == QEvent::Resize)
        {
            if(m_position < Bottom)
                resize(width(), m_mainWindow->height()-32);
            else
                resize(m_mainWindow->width()-32, height());
            adjustPosition();
            return false;
        }
        else if (event->type() == QEvent::Move)
            if (event->spontaneous())
                adjustPosition();
    return false;
#endif
    return QDockWidget::eventFilter(object, event);
}

void
DockWidget::paintEvent(QPaintEvent *event)
{
    QDockWidget::paintEvent(event);
#if 0
    if (!isFloating())
        return;
    QPainter p(this);
    p.fillRect(rect(), Operations::colorMid(palette().color(QPalette::Window), palette().color(QPalette::WindowText), 10, 1));
    p.translate(0.5, 0.5);
    QColor pc(palette().color(QPalette::WindowText));
    pc.setAlpha(192);
    p.setPen(pc);
    p.setRenderHint(QPainter::Antialiasing);
    QRect r(rect().adjusted(0, 0, -1, -1));
    p.setPen(QColor(255, 255, 255, 96));
    p.drawRoundedRect(r, 5, 5);
    p.setPen(pc);
    p.setBrush(palette().color(QPalette::Window));
    r.adjust(m_margin, m_margin, -m_margin, -m_margin);
    p.drawRect(r);
    r.setTop(r.top()+m_titleWidget->height());
    p.drawLine(r.topLeft(), r.topRight());
    p.end();
#endif
}

void
DockWidget::moveEvent(QMoveEvent *event)
{
//    qDebug() << mainWin->height() << geometry().height();
//    if (event->spontaneous())
//    {
//        switch (myPos)
//        {
//        case Left:
//            mainWin->move(geometry().right()-margin, geometry().top()-(16+margin*2)); break;
//        case Right:
//            mainWin->move(geometry().left()-mainWin->width(), geometry().top()-16); break;
//        case Bottom:
//            mainWin->move(geometry().left()-16, geometry().top()-mainWin->height()); break;
//        }
//    event->accept();
//    }
#if 0
    if (isFloating())
    {
        event->accept();
        adjustPosition();
    }
#endif
}

void
DockWidget::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);
#if 0
    if (!isFloating())
        return;
    if (m_position < Bottom)
        resize(width(), m_mainWindow->height()-32);
    else
        resize(m_mainWindow->width()-32, height());

    setMask(shape());
    adjustPosition();
#endif
}

void
DockWidget::animate()
{
#if 0
    if (m_position < Bottom && m_animStep <= (width()-m_margin) || m_position == Bottom && m_animStep <= (height()-m_margin))
        m_animStep += 2;
    else
    {
        m_timer->stop();
        m_animStep = 0;
    }
    adjustPosition();
#endif
}

void
DockWidget::adjustPosition()
{
#if 0
    QPoint p;
    switch (m_position)
    {
    case Left:
    {
        p = QPoint(m_mainWindow->geometry().topLeft());
        if (m_animStep && m_dirIn)
            p.setX(p.x()-(width()-(m_margin+1))+m_animStep);
        else if (m_animStep)
            p.setX(p.x()-qMin(m_animStep, (width()-(m_margin+1))));
        else
            p.setX(p.x()-(width()-(m_margin+1)));
        break;
    }
    case Right:
    {
        p = QPoint(m_mainWindow->geometry().topRight());
        if (m_animStep && m_dirIn)
            p.setX(p.x()-m_animStep);
        else if (m_animStep)
            p.setX((p.x()-(width()))+m_animStep);
        else
            p.setX(p.x()-m_margin);
        break;
    }
    case Bottom:
    {
        p = QPoint(m_mainWindow->geometry().bottomLeft());
        if (m_animStep && m_dirIn)
            p.setY(p.y()-m_animStep);
        else if (m_animStep)
            p.setY(p.y()-(height()-m_animStep));
        else
            p.setY(p.y()-m_margin);
        break;
    }
    }
    if (m_position < Bottom)
        p.setY(p.y()+((m_mainWindow->height()-height())/2));
    else
        p.setX(p.x()+16);
    move(p);
    if(m_dirIn && (m_position < Bottom && m_animStep >= (width()-m_margin) || m_position == Bottom && m_animStep >= (height()-m_margin)))
    {
        setFloating(false);
        m_dirIn = false;
    }
#endif
}
