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


#ifndef INTERFACES_H
#define INTERFACES_H


#include <QtPlugin>

class QImage;
class QString;
class QStringList;
class ThumbInterface
{
public:
    virtual ~ThumbInterface() {}
    virtual void init() = 0;
    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual bool thumb(const QString &file, const QString &mime, QImage &thumb, const int size = 256) = 0;
};

Q_DECLARE_INTERFACE(ThumbInterface, "dfm.ThumbInterface/0.01")

#endif // INTERFACES_H
