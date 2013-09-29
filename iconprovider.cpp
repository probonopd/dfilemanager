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


#include "iconprovider.h"

#include <QDebug>

using namespace DFM;

const QPolygon
IconProvider::triangle(bool back, int size)
{
    const int sz = size/4;
    QPolygon pol;
    if(back)
        pol.setPoints(3, size-sz, sz, size-sz, size-sz, sz, size/2);
    else
        pol.setPoints(3, sz, sz, sz, size-sz, size-sz, size/2);
    return pol;
}

const QIcon
IconProvider::icon(Type type, int size, QColor color, bool themeIcon)
{
    if(themeIcon)
    {
        QString iconString;
        switch (type)
        {
        case IconView : iconString = "view-list-icons"; break;
        case DetailsView : iconString  = "view-list-details"; break;
        case ColumnsView : iconString = "view-file-columns"; break;
        case FlowView : iconString = "view-preview"; break;
        case GoBack : iconString = "go-previous"; break;
        case GoForward : iconString = "go-next"; break;
        case Configure : iconString = "configure"; break;
        case GoHome : iconString = "go-home"; break;
        case Search : iconString = "edit-find"; break;
        case Clear : iconString = "list-remove"; break;
        default : iconString = "";
        }

        if(!QIcon::fromTheme(iconString).isNull())
            return QIcon::fromTheme(iconString);
    }

    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    QRect rect(pix.rect());
    int min = size / 16, sz = (size/4)-min;
    p.setPen(color);
    switch (type)
    {
    case IconView :
    {
        const int x = rect.left(), y = rect.top(), r = rect.right(), b = rect.bottom();
        rect.setSize(QSize(sz, sz));
        rect.moveTopLeft(QPoint(x+sz, y+sz));
        p.drawRect(rect);
        rect.moveRight(r-sz);
        p.drawRect(rect);
        rect.moveBottom(b-(sz+min));
        p.drawRect(rect);
        rect.moveLeft(x+sz);
        p.drawRect(rect);
        break;
    }
    case DetailsView :
        for(int i = sz; i<size-sz; i+=sz)
            p.drawLine(sz, i, size-(sz+1), i);
        break;
    case ColumnsView :
    {
        size = size - size/16;
        sz = size / 4;
        const int w = (size)/3;
        for(int i = 0; i <= size; i += w)
            p.drawLine(i, sz, i, size-sz);
        p.drawLine(0, sz, size, sz);
        p.drawLine(0, size-sz, size, size-sz);
        break;
    }
    case FlowView :
    {
        size = size - size/16;
        sz = size / 4;
        QRect rect(0, sz, size, size-sz*2);
        p.drawLine(rect.topLeft(), rect.bottomLeft());
        p.drawLine(rect.topRight(), rect.bottomRight());
        rect.adjust(2, 1, -2, -1);
        p.drawLine(rect.topLeft(), rect.bottomLeft());
        p.drawLine(rect.topRight(), rect.bottomRight());
        rect.adjust(2, 0, -2, -0);
        p.fillRect(rect, color);
        break;
    }
    case GoBack :
    case GoForward :
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.setRenderHint(QPainter::Antialiasing);
        p.drawPolygon(triangle(type == GoBack, size));
        break;
    case Configure : //somekinda gearwheel
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        const int _1pt = size/16, _2pt = size/8, _4pt = size/4, _8pt = size/2;

        QPainterPath path;
        path.moveTo( _8pt - _1pt, _1pt );
        path.lineTo( _8pt + _1pt, _1pt );
        path.lineTo( _8pt + _2pt, _8pt );
        path.lineTo( _8pt - _2pt, _8pt );
        path.lineTo( _8pt - _1pt, _1pt );
        path.closeSubpath();

        for(int i = 0; i < 360; i+= 45)
        {

            p.translate( size/2, size/2 );
            p.rotate( 45 );
            p.translate(-(size/2), -(size/2));
            p.drawPath( path );

        }
        p.resetTransform();
        const int outer = _4pt-_1pt;
        p.setPen(Qt::NoPen);
        p.drawEllipse( pix.rect().adjusted(outer, outer, -outer, -outer ) );
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setBrush(Qt::black);
        p.drawEllipse( pix.rect().adjusted( _8pt-_2pt, _8pt-_2pt, -(_8pt-_2pt), -(_8pt-_2pt) ) );
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.drawEllipse(pix.rect().adjusted( _1pt, _1pt, -_1pt, -_1pt) );
        break;
    }
    case GoHome :
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(0.80, 0.80);
        p.translate(size*0.1, size*0.1);
        p.setClipRect(rect);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(0, size-sz*2, size, size, size/3, size/3);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        QRect r(size/4, 0, size/2, size-size/4);
        p.drawEllipse(r);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        const int a = size/16;
        r.adjust(a, a, -a, -a);
        p.drawEllipse(r);
        break;
    }
    case Search :
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        QRect r(rect.adjusted(size/16, size/16, -size/4, -size/4));
        p.drawEllipse(r);
        p.setPen(QPen(color, size/8));
        p.drawLine(QPoint(size-size/8, size-size/8), r.center());
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        const int d = size/5;
        r.adjust(d, d, -d, -d);
        p.drawEllipse(r);
        break;
    }
    case Clear :
    {
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(rect);
        QRect c(rect.adjusted(size/4+1, size/4+1, -size/4, -size/4));
        p.setPen(QPen(color, (float)size/8.0));
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.drawLine(c.topLeft(), c.bottomRight());
        p.drawLine(c.topRight(), c.bottomLeft());
        break;
    }
    }
    p.end();
    return QIcon(pix);
}
