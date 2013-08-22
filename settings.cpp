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


#include "mainwindow.h"
#include "iconprovider.h"

using namespace DFM;

void
MainWindow::readSettings()
{
    restoreState(Configuration::settings()->value("windowState").toByteArray(), 1);
    m_tabWin->restoreState(Configuration::settings()->value("tabWindowState").toByteArray(), 1);
    resize(Configuration::settings()->value("size", QSize(800, 300)).toSize());
    move(Configuration::settings()->value("pos", QPoint(200, 200)).toPoint());

    m_tabWin->addDockWidget(Qt::LeftDockWidgetArea, m_dockLeft);
    m_tabWin->addDockWidget(Qt::RightDockWidgetArea, m_dockRight);
    m_tabWin->addDockWidget(Qt::BottomDockWidgetArea, m_dockBottom);

    m_statAct->setChecked(Configuration::settings()->value("statusVisible", true).toBool());
    m_pathVisibleAct->setChecked(Configuration::settings()->value("pathVisible", true).toBool());
    m_pathEditAct->setChecked(Configuration::settings()->value("pathEditable", false).toBool());
    m_menuAct->setChecked(Configuration::settings()->value("menuVisible", true).toBool());
    m_placesView->populate();
    updateConfig();
}

void
MainWindow::updateIcons()
{
    const QColor &tbfgc(m_navToolBar->palette().color(m_navToolBar->foregroundRole()));
    const int &tbis = m_navToolBar->iconSize().height();
    m_homeAct->setIcon(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_goBackAct->setIcon(IconProvider::icon(IconProvider::GoBack, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_goForwardAct->setIcon(IconProvider::icon(IconProvider::GoForward, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_iconViewAct->setIcon(IconProvider::icon(IconProvider::IconView, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_listViewAct->setIcon(IconProvider::icon(IconProvider::DetailsView, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_colViewAct->setIcon(IconProvider::icon(IconProvider::ColumnsView, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_flowAct->setIcon(IconProvider::icon(IconProvider::FlowView, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_homeAct->setIcon(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    m_configureAct->setIcon(IconProvider::icon(IconProvider::Configure, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
}

void
MainWindow::updateConfig()
{
    m_tabBar->setVisible(m_tabBar->count() > 1 ? true : !Configuration::config.behaviour.hideTabBarWhenOnlyOneTab);
    if ( m_activeContainer )
        m_activeContainer->refresh();
    updateIcons();
}

void
MainWindow::writeSettings()
{
    Configuration::settings()->setValue("pos", pos());
    Configuration::settings()->setValue("size", size());
    Configuration::settings()->setValue("windowState", saveState(1));
    Configuration::settings()->setValue("tabWindowState", m_tabWin->saveState(1));
    Configuration::settings()->setValue("statusVisible", m_statAct->isChecked());
    Configuration::settings()->setValue("pathVisible", m_pathVisibleAct->isChecked());
    Configuration::settings()->setValue("pathEditable", m_pathEditAct->isChecked());
    Configuration::settings()->setValue("menuVisible", m_menuAct->isChecked());
    Configuration::writeConfig();
    m_placesView->store();
}
