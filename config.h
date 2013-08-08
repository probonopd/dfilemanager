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


#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

namespace DFM
{

typedef struct Config
{
    QString startPath;
    struct behaviour
    {
        bool hideTabBarWhenOnlyOneTab, systemIcons, devUsage;
        int view;
    } behaviour;
    struct views
    {
        bool showThumbs;
        struct iconView
        {
            bool smoothScroll;
            int textWidth;
        } iconView;
        struct detailsView
        {
            int rowPadding;
        } detailsView;
    } views;

    struct docks
    {
        int lock;
    } docks;
} Config;

}

#endif // CONFIG_H
