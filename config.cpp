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


#include "config.h"
#include <QDir>

using namespace DFM;

static Configuration *inst = 0;
Config Configuration::config;

Configuration::Configuration()
{
    m_settings = new QSettings("dfm", "dfm");
}

Configuration
*Configuration::instance()
{
    if ( !inst )
        inst = new Configuration();
    return inst;
}

void
Configuration::readConfiguration()
{
    config.docks.lock = settings()->value( "docks.lock", 0 ).toInt();
    config.startPath = settings()->value("startPath", QDir::homePath()).toString();
    config.behaviour.hideTabBarWhenOnlyOneTab = settings()->value("hideTabBarWhenOnlyOne", false).toBool();
    config.behaviour.systemIcons = settings()->value("useSystemIcons", false).toBool();
    config.views.iconView.smoothScroll = settings()->value("smoothScroll", false).toBool();
    config.views.showThumbs = settings()->value("showThumbs", false).toBool();
    config.behaviour.devUsage = settings()->value("drawDevUsage", false).toBool();
    config.views.iconView.textWidth = settings()->value("textWidth", 16).toInt();
    config.views.detailsView.rowPadding = settings()->value("detailsView.rowPadding", 0).toInt();
    config.behaviour.view = settings()->value("start.view", 0).toInt();
    config.views.iconView.iconSize = settings()->value("iconView.iconSize", 3).toInt();
    config.views.flowSize = settings()->value("flowSize", QByteArray()).toByteArray();
    config.styleSheet = settings()->value("styleSheet", QString()).toString();
}

void
Configuration::writeConfiguration()
{
    settings()->setValue("startPath", config.startPath);
    settings()->setValue("docks.lock", config.docks.lock);
    settings()->setValue("smoothScroll", config.views.iconView.smoothScroll);
    settings()->setValue("showThumbs", config.views.showThumbs);
    settings()->setValue("drawDevUsage", config.behaviour.devUsage);
    settings()->setValue("textWidth", config.views.iconView.textWidth);
    settings()->setValue("detailsView.rowPadding", config.views.detailsView.rowPadding);
    settings()->setValue("start.view", config.behaviour.view);
    settings()->setValue("iconView.iconSize", config.views.iconView.iconSize);
    settings()->setValue("flowSize", config.views.flowSize);
    settings()->setValue("hideTabBarWhenOnlyOne", config.behaviour.hideTabBarWhenOnlyOneTab);
    settings()->setValue("styleSheet", config.styleSheet);
}
