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
MainWindow::readConfig()
{
    config.docks.lock = m_settings->value( "docks.lock", 0 ).toInt();
    config.startPath = m_settings->value("startPath", QDir::homePath()).toString();
    config.behaviour.hideTabBarWhenOnlyOneTab = m_settings->value("hideTabBarWhenOnlyOne", false).toBool();
    config.behaviour.systemIcons = m_settings->value("useSystemIcons", false).toBool();
    config.views.iconView.smoothScroll = m_settings->value("smoothScroll", false).toBool();
    config.views.showThumbs = m_settings->value("showThumbs", false).toBool();
    config.behaviour.devUsage = m_settings->value("drawDevUsage", false).toBool();
    config.views.iconView.textWidth = m_settings->value("textWidth", 16).toInt();
}

void
MainWindow::readSettings()
{
    restoreState(m_settings->value("windowState").toByteArray(), 1);
    m_tabWin->restoreState(m_settings->value("tabWindowState").toByteArray(), 1);
    //  splitter->restoreState(settings->value("mainSplitter").toByteArray());
    QPoint pos = m_settings->value("pos", QPoint(200, 200)).toPoint();
    QSize size = m_settings->value("size", QSize(800, 300)).toSize();
    resize(size);
    move(pos);

    m_tabWin->addDockWidget(Qt::LeftDockWidgetArea, m_dockLeft);
    m_tabWin->addDockWidget(Qt::RightDockWidgetArea, m_dockRight);
    m_tabWin->addDockWidget(Qt::BottomDockWidgetArea, m_dockBottom);

    m_statAct->setChecked(m_settings->value("statusVisible", true).toBool());
    m_pathVisibleAct->setChecked(m_settings->value("pathVisible", true).toBool());
    m_pathEditAct->setChecked(m_settings->value("pathEditable", false).toBool());
    m_menuAct->setChecked(m_settings->value("menuVisible", true).toBool());

    m_settings->beginGroup("Places");
    m_placesView->clear(); //should be superfluous but still need to make sure
    foreach ( const QString &key, m_settings->childKeys() )
    {
        const QStringList &values(m_settings->value(key).toStringList());
        //      if (values.count() != 4)
        //          break;

        if (QString(values[2]).isEmpty())
            new QTreeWidgetItem(m_placesView, values);
        else
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_placesView->findItems(values[2], Qt::MatchExactly).first(), values);
            item->setIcon(0, QIcon::fromTheme(values[PlacesView::DevPath]));
        }
    }
    m_settings->endGroup();
    m_settings->beginGroup("Expandplaces");
    for (int i = 0; i < m_placesView->topLevelItemCount(); i++)
        m_placesView->topLevelItem(i)->setExpanded(m_settings->value(m_placesView->topLevelItem(i)->text(0)).toBool());
    m_settings->endGroup();

    updateConfig();
}

void
MainWindow::updateIcons()
{
    const QColor &tbfgc(m_navToolBar->palette().color(m_navToolBar->foregroundRole()));
    const int &tbis = m_navToolBar->iconSize().height();
    m_homeAct->setIcon(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, config.behaviour.systemIcons));
    m_goBackAct->setIcon(IconProvider::icon(IconProvider::GoBack, tbis, tbfgc, config.behaviour.systemIcons));
    m_goForwardAct->setIcon(IconProvider::icon(IconProvider::GoForward, tbis, tbfgc, config.behaviour.systemIcons));
    m_iconViewAct->setIcon(IconProvider::icon(IconProvider::IconView, tbis, tbfgc, config.behaviour.systemIcons));
    m_listViewAct->setIcon(IconProvider::icon(IconProvider::DetailsView, tbis, tbfgc, config.behaviour.systemIcons));
    m_colViewAct->setIcon(IconProvider::icon(IconProvider::ColumnsView, tbis, tbfgc, config.behaviour.systemIcons));
    m_flowAct->setIcon(IconProvider::icon(IconProvider::FlowView, tbis, tbfgc, config.behaviour.systemIcons));
    m_homeAct->setIcon(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, config.behaviour.systemIcons));
    m_configureAct->setIcon(IconProvider::icon(IconProvider::Configure, tbis, tbfgc, config.behaviour.systemIcons));
}

void
MainWindow::updateConfig()
{
    readConfig();
    m_tabBar->setVisible(m_tabBar->count() > 1 ? true : !config.behaviour.hideTabBarWhenOnlyOneTab);
    updateIcons();
}

void
MainWindow::writeSettings()
{
    m_settings->setValue("pos", pos());
    m_settings->setValue("size", size());
    m_settings->setValue("windowState", saveState(1));
    m_settings->setValue("tabWindowState", m_tabWin->saveState(1));
    m_settings->setValue("statusVisible", m_statAct->isChecked());
    m_settings->setValue("pathVisible", m_pathVisibleAct->isChecked());
    m_settings->setValue("pathEditable", m_pathEditAct->isChecked());
    m_settings->setValue("menuVisible", m_menuAct->isChecked());
    m_settings->setValue("docks.lock", config.docks.lock);
    m_settings->setValue("smoothScroll", config.views.iconView.smoothScroll);
    m_settings->setValue("showThumbs", config.views.showThumbs);
    m_settings->setValue("drawDevUsage", config.behaviour.devUsage);
    m_settings->setValue("textWidth", config.views.iconView.textWidth);

    m_settings->remove("Places");
    m_settings->beginGroup("Places");
    int placeNr = 0;
    QTreeWidgetItemIterator it(m_placesView);
    while (*it)
    {
        const QTreeWidgetItem *item = *it;
        if (item->text(2) != "Devices") //we dont save devices... we populate those everytime
            m_settings->setValue(QString(placeNr), QStringList() << item->text(0) << item->text(1) << (item->parent() ? item->parent()->text(0) : "") << (item->text(3).isEmpty() ? "folder" : item->text(3)) );
        ++placeNr;
        ++it;
    }
    m_settings->endGroup();
    m_settings->remove("ExpandPlaces");
    m_settings->beginGroup("Expandplaces");
    for (int i = 0; i < m_placesView->topLevelItemCount(); i++)
        m_settings->setValue(m_placesView->topLevelItem(i)->text(0), m_placesView->topLevelItem(i)->isExpanded());
    m_settings->endGroup();
}
