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


#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <QString>
#include <QColor>
#include "Proxystyle/proxystyle.h"

namespace DFM
{

class MyStyle : public ProxyStyle
{
public:
   inline explicit MyStyle(const QString& baseStyle) : ProxyStyle(baseStyle) {}
   inline virtual int pixelMetric(PixelMetric metric, const QStyleOption* option = 0, const QWidget* widget = 0) const
   {
       if (metric == PM_DockWidgetFrameWidth)
           return 4;
       else if (metric == PM_ToolBarFrameWidth)
           return 0;
       return ProxyStyle::pixelMetric(metric, option, widget);
   }
};

class Operations
{
public:
    Operations(){}
    static QString getMimeType(const QString &file);
    static QString getFileType(const QString &file);
    enum Usage{Free = 0, Used, Total, Id};
    static quint64 getDriveInfo(const QString &file, const Usage &t);
    static QColor colorMid(const QColor c1, const QColor c2, int i1 = 1, int i2 = 1);
    static void openFile( const QString &file );
};

}

#endif // OPERATIONS_H
