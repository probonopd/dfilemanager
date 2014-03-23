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


#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QIcon>
#include <QPixmap>
#include <QImage>
#include <QPainter>

namespace DFM
{
namespace IconProvider
{
const QPolygon triangle(bool back, int size);

enum Type { IconView = 0,
            DetailsView,
            ColumnsView,
            FlowView,
            GoBack,
            GoForward,
            Configure,
            GoHome,
            Search,
            Clear,
            Sort,
            Hidden,
            Animator,
            OK,
            Circle,
            CloseTab,
            NewTab
          };

const QIcon icon(Type type, int size, QColor color, bool themeIcon);
}

}

#endif // ICONPROVIDER_H
