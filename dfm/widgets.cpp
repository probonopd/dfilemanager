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


#include <QPainter>
#include <QMouseEvent>
#include "widgets.h"
#include "iconprovider.h"

using namespace DFM;

#define STEPS 24
#define INTERVAL 40

Button::Button(QWidget *parent)
    : QWidget(parent)
    , m_hasMenu(false)
    , m_hasPress(false)
    , m_isAnimating(false)
    , m_menu(0)
    , m_animStep(0)
    , m_animTimer(new QTimer(this))
{
    m_stepSize = 360/STEPS;
    connect(m_animTimer, SIGNAL(timeout()), this, SLOT(animate()));
    m_animIcon = IconProvider::icon(IconProvider::Animator, 16, palette().color(foregroundRole()), false);
}

void
Button::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    m_hasPress = true;
}

void
Button::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);
    if (m_hasPress && rect().contains(e->pos()))
    {
        if (m_hasMenu)
            m_menu->popup(mapToGlobal(e->pos()));
        else
            emit clicked();
    }
    m_hasPress = false;
}

void
Button::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter p(this);
    if (m_isAnimating)
    {
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setTransform(m_transform);
        m_animIcon.paint(&p, rect(), Qt::AlignCenter, isEnabled()?QIcon::Normal:QIcon::Disabled);
    }
    else if (!m_icon.isNull())
        m_icon.paint(&p, rect(), Qt::AlignCenter, isEnabled()?QIcon::Normal:QIcon::Disabled);
    p.end();
}

void
Button::animate()
{
    const int x = width()/2, y = height()/2;
    m_transform.translate(x, y);
    m_transform.rotate(m_stepSize);
    m_transform.translate(-x, -y);
    update();
}

void
Button::startAnimating()
{
    m_transform.reset();
    m_isAnimating=true;
    m_animStep=0;
    m_animTimer->start(INTERVAL);
    update();
}

void
Button::stopAnimating()
{
    m_isAnimating=false;
    m_animTimer->stop();
    update();
}

void
Button::setIcon(const QIcon &icon)
{
    m_icon = icon;
    update();
}
#if QT_VERSION < 0x050000
void
Button::paletteChange(const QPalette &p)
{
    QWidget::paletteChange(p);
    m_animIcon = IconProvider::icon(IconProvider::Animator, 16, palette().color(foregroundRole()), false);
    update();
}
#endif
