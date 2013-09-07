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


#include "searchbox.h"
#include "iconprovider.h"
#include "config.h"
#include "mainwindow.h"

using namespace DFM;


IconWidget::IconWidget(QWidget *parent) : QWidget(parent), m_opacity(0.5), m_type(false)
{
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
    m_pix[0] = IconProvider::icon(IconProvider::Search, 16, palette().color(foregroundRole()), Store::config.behaviour.systemIcons).pixmap(16);
    m_pix[1] = IconProvider::icon(IconProvider::Clear, 16, palette().color(foregroundRole()), Store::config.behaviour.systemIcons).pixmap(16);
}

void
IconWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setOpacity(m_opacity);
    p.drawTiledPixmap(rect(), m_pix[m_type]);
    p.end();
}

SearchBox::SearchBox(QWidget *parent) : QLineEdit(parent), m_margin(4), m_iconWidget(new IconWidget(this))
{
    setPlaceholderText("Filter By Name");
    setTextMargins(m_iconWidget->rect().width()+m_margin, 0, 0, 0);
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setFixedWidth(222);

    connect( this, SIGNAL(textChanged(QString)), this, SLOT(setClearButtonEnabled(QString)));
    connect( m_iconWidget, SIGNAL(clicked()), this, SLOT(clear()));
}

void
SearchBox::resizeEvent(QResizeEvent *event)
{
    m_iconWidget->move(rect().left() + m_margin, (rect().bottom()-m_iconWidget->rect().bottom())/2);
    QLineEdit::resizeEvent(event);
}
