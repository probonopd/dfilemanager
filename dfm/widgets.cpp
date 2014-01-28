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

using namespace DFM;

Button::Button(QWidget *parent)
    : QWidget(parent)
    , m_hasMenu(false)
    , m_hasPress(false)
    , m_menu(0)
{}

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
    m_icon.paint(&p, rect(), Qt::AlignCenter, isEnabled()?QIcon::Normal:QIcon::Disabled);
    p.end();
}
