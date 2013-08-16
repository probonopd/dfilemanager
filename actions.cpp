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
#include <QMenuBar>

using namespace DFM;

void
MainWindow::createActions()
{
    QColor tbfgc(m_navToolBar->palette().color(m_navToolBar->foregroundRole()));
    int tbis = m_navToolBar->iconSize().height();
    m_delCurrentSelectionAct = new QAction(QIcon::fromTheme("edit-delete"), tr("&Delete"), this);
    //    delCurrentSelectionAct->setShortcuts( QList<QKeySequence>() << QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    m_delCurrentSelectionAct->setShortcut(QKeySequence("Shift+Del"));
    m_delCurrentSelectionAct->setShortcutContext( Qt::ApplicationShortcut );
    m_delCurrentSelectionAct->setObjectName("actionDelete");
    connect(m_delCurrentSelectionAct, SIGNAL(triggered()), this, SLOT(deleteCurrentSelection()));

    m_homeAct = new QAction(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Go Home"), this);
    //  homeAct->setStatusTip(tr("Go to ur home dir"));
    m_homeAct->setObjectName("actionHome");
    connect(m_homeAct, SIGNAL(triggered()), this, SLOT(goHome()));

    m_goBackAct = new QAction( IconProvider::icon(IconProvider::GoBack, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Go Back"), this);
    m_goBackAct->setShortcuts( QKeySequence::Back);
    m_goBackAct->setShortcutContext( Qt::ApplicationShortcut );
    m_goBackAct->setObjectName("actionGoBack");
    //  goBackAct->setStatusTip(tr("Go to previous folder"));
    connect(m_goBackAct, SIGNAL(triggered()), this, SLOT(goBack()));

    m_goUpAct = new QAction(QIcon::fromTheme("go-up"), tr("&Go Up"), this);
    m_goUpAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Alt+Up") );
    m_goUpAct->setShortcutContext( Qt::ApplicationShortcut );
    m_goUpAct->setObjectName("actionGoUp");
    connect(m_goUpAct, SIGNAL(triggered()), this, SLOT(goUp()));

    m_goForwardAct = new QAction(IconProvider::icon(IconProvider::GoForward, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Go Forward"), this);
    m_goForwardAct->setShortcuts(QKeySequence::Forward);
    m_goForwardAct->setShortcutContext( Qt::ApplicationShortcut );
    m_goForwardAct->setObjectName("actionGoForward");
    //  goForwardAct->setStatusTip(tr("Go to next folder"));
    connect(m_goForwardAct, SIGNAL(triggered()), this, SLOT(goForward()));

    m_exitAct = new QAction(tr("E&xit"), this);
    m_exitAct->setShortcuts(QKeySequence::Quit);
    m_exitAct->setShortcutContext( Qt::ApplicationShortcut );
    m_exitAct->setObjectName("actionExit");
    //  exitAct->setStatusTip(tr("Exit the application"));
    connect(m_exitAct, SIGNAL(triggered()), this, SLOT(close()));

    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setObjectName("actionAbout");
    //  aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    m_aboutQtAct = new QAction(tr("About &Qt"), this);
    m_aboutQtAct->setObjectName("actionAboutQt");
    //  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(m_aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    m_placeAct = new QAction(QIcon::fromTheme("bookmark-new"),tr("&Add Place"), this);
//    placeAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+d") );
//    placeAct->setShortcutContext( Qt::ApplicationShortcut );
    m_placeAct->setObjectName("actionPlace");
    connect(m_placeAct, SIGNAL(triggered()), this, SLOT(genPlace()));

    m_renPlaceAct = new QAction(QIcon::fromTheme("bookmarks-organize"),tr("&Rename Place"), this);
    //  renPlaceAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+d") );
    //  renPlaceAct->setShortcutContext( Qt::ApplicationShortcut );
    m_renPlaceAct->setObjectName("actionRenPlace");
    connect(m_renPlaceAct, SIGNAL(triggered()), m_placesView, SLOT(renPlace()));

    m_placeContAct = new QAction(QIcon::fromTheme("bookmark-new"),tr("&Add Place Container"), this);
//    placeContAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+Alt+P") );
//    placeContAct->setShortcutContext( Qt::ApplicationShortcut );
    m_placeContAct->setObjectName("actionPlaceContainer");
    connect(m_placeContAct, SIGNAL(triggered()), m_placesView, SLOT(addPlaceCont()));

    m_rmPlaceAct = new QAction(QIcon::fromTheme("list-remove"),tr("&Remove Place"), this);
//    rmPlaceAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+Shift+D") );
//    rmPlaceAct->setShortcutContext( Qt::ApplicationShortcut );
    m_rmPlaceAct->setObjectName("actionRmPlace");
    connect(m_rmPlaceAct, SIGNAL(triggered()), m_placesView, SLOT(removePlace()));

    m_iconViewAct = new QAction(IconProvider::icon(IconProvider::IconView, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Icons View"), this);
    m_iconViewAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+1") );
    m_iconViewAct->setShortcutContext( Qt::ApplicationShortcut );
    m_iconViewAct->setObjectName("actionIconView");
    m_iconViewAct->setCheckable(true);
    m_iconViewAct->setChecked(true);
    connect(m_iconViewAct, SIGNAL(triggered()), this, SLOT(setViewIcons()));

    m_listViewAct = new QAction(IconProvider::icon(IconProvider::DetailsView, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Details View"), this);
    m_listViewAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+2") );
    m_listViewAct->setShortcutContext( Qt::ApplicationShortcut );
    m_listViewAct->setObjectName("actionsListView");
    m_listViewAct->setCheckable(true);
    connect(m_listViewAct, SIGNAL(triggered()), this, SLOT(setViewDetails()));

    m_colViewAct = new QAction(IconProvider::icon(IconProvider::ColumnsView, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Columns View"), this);
    m_colViewAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+3") );
    m_colViewAct->setShortcutContext( Qt::ApplicationShortcut );
    m_colViewAct->setObjectName("actionColView");
    m_colViewAct->setCheckable(true);
    connect(m_colViewAct, SIGNAL(triggered()), this, SLOT(setViewCols()));

    m_flowAct = new QAction(IconProvider::icon(IconProvider::FlowView, tbis, tbfgc, Configuration::config.behaviour.systemIcons), tr("&Flow"), this);
    m_flowAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+4") );
    m_flowAct->setShortcutContext( Qt::ApplicationShortcut );
    m_flowAct->setObjectName("actionFlow");
    m_flowAct->setCheckable(true);
    connect(m_flowAct, SIGNAL(triggered()), this, SLOT(flowView()));

    m_showHiddenAct = new QAction(tr("&Show Hidden"), this);
    m_showHiddenAct->setShortcuts( QList<QKeySequence>() << QKeySequence("F8") );
    m_showHiddenAct->setShortcutContext( Qt::ApplicationShortcut );
    m_showHiddenAct->setObjectName("actionShowHidden");
    m_showHiddenAct->setCheckable(true);
    connect(m_showHiddenAct, SIGNAL(triggered()), this, SLOT(toggleHidden()));

    m_mkDirAct = new QAction(QIcon::fromTheme("folder-new"), tr("&Create Directory"), this);
    m_mkDirAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F10") );
    m_mkDirAct->setShortcutContext( Qt::ApplicationShortcut );
    m_mkDirAct->setObjectName("actionMkDir");
    connect(m_mkDirAct, SIGNAL(triggered()), this, SLOT(createDirectory()));

    m_copyAct = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    m_copyAct->setShortcuts(QKeySequence::Copy);
    m_copyAct->setShortcutContext( Qt::ApplicationShortcut );
    m_copyAct->setObjectName("actionCopy");
    connect(m_copyAct, SIGNAL(triggered()), this, SLOT(setClipBoard()));

    m_cutAct = new QAction(QIcon::fromTheme("edit-cut"), tr("&Cut"), this);
    m_cutAct->setShortcut(QKeySequence("Ctrl+X"));
    m_cutAct->setShortcutContext( Qt::ApplicationShortcut );
    m_cutAct->setObjectName("actionCut");
    connect(m_cutAct, SIGNAL(triggered()), this, SLOT(cutSelection()));

    m_pasteAct = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    m_pasteAct->setShortcuts(QKeySequence::Paste);
    m_pasteAct->setShortcutContext( Qt::ApplicationShortcut );
    m_pasteAct->setObjectName("actionPaste");
    connect(m_pasteAct, SIGNAL(triggered()), this, SLOT(pasteSelection()));

    m_renameAct = new QAction(QIcon::fromTheme("edit-rename"), tr("&Rename"), this);
    m_renameAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F2") );
    m_renameAct->setShortcutContext( Qt::ApplicationShortcut );
    m_renameAct->setObjectName("actionRename");
    connect(m_renameAct, SIGNAL(triggered()), this, SLOT(rename()));

    m_refreshAct = new QAction(QIcon::fromTheme("view-refresh"), tr("&Refresh"), this);
    m_refreshAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F5") );
    m_refreshAct->setShortcutContext( Qt::ApplicationShortcut );
    m_refreshAct->setObjectName("acgtionRefresh");
    connect(m_refreshAct, SIGNAL(triggered()), this, SLOT(refreshView()));

    m_placeIconAct = new QAction(tr("&Gustom Icon"), this);
    m_placeIconAct->setObjectName("actionPlaceIcon");
    connect(m_placeIconAct, SIGNAL(triggered()), m_placesView, SLOT(setPlaceIcon()));

    m_cstCmdAct = new QAction(tr("&Open With Cmd"), this);
    m_cstCmdAct->setObjectName("actionCustomCmd");
    connect(m_cstCmdAct, SIGNAL(triggered()), this, SLOT(customCommand()));

    m_menuAct = new QAction(tr("Show MenuBar"),this);
    m_menuAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+M") );
    m_menuAct->setShortcutContext( Qt::ApplicationShortcut );
    m_menuAct->setObjectName("actionMenu");
    m_menuAct->setCheckable(true);
    connect(m_menuAct,SIGNAL(triggered()),this, SLOT(toggleMenuVisible()));

    m_statAct = new QAction(tr("Show StatusBar"),this);
    m_statAct->setObjectName("actionStatus");
    m_statAct->setCheckable(true);
    connect(m_statAct,SIGNAL(triggered()),this, SLOT(toggleStatusVisible()));

    m_pathVisibleAct = new QAction(tr("Show Path"),this);
    m_pathVisibleAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+P") );
    m_pathVisibleAct->setShortcutContext( Qt::ApplicationShortcut );
    m_pathVisibleAct->setObjectName("actionPathVisible");
    m_pathVisibleAct->setCheckable(true);
    connect(m_pathVisibleAct, SIGNAL(triggered()),this,SLOT(hidePath()));

    m_addTabAct = new QAction(tr("New Tab"),this);
    m_addTabAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+T") );
    m_addTabAct->setShortcutContext( Qt::ApplicationShortcut );
    m_addTabAct->setObjectName("actionAddTab");
    connect(m_addTabAct,SIGNAL(triggered()),this,SLOT(newTab()));

    m_openInTabAct = new QAction(tr("Open In New Tab"),this);
    m_openInTabAct->setObjectName("actionOpenInTab");
    connect(m_openInTabAct,SIGNAL(triggered()),this,SLOT(openTab()));

    m_configureAct = new QAction(tr("Configure"),this);
    m_configureAct->setObjectName("configAct");
    m_configureAct->setIcon(IconProvider::icon(IconProvider::Configure, tbis, tbfgc, Configuration::config.behaviour.systemIcons));
    connect(m_configureAct, SIGNAL(triggered()),this,SLOT(showSettings()));

    m_propertiesAct = new QAction(tr("Properties"),this);
    m_propertiesAct->setObjectName("propAct");
    m_propertiesAct->setIcon(QIcon::fromTheme("Configure"));
    connect(m_propertiesAct, SIGNAL(triggered()),this,SLOT(fileProperties()));

    m_pathEditAct = new QAction(tr("Edit Path"),this);
    m_pathEditAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F6") );
    m_pathEditAct->setShortcutContext( Qt::ApplicationShortcut );
    m_pathEditAct->setObjectName("actionEditPath");
    m_pathEditAct->setCheckable(true);
    connect(m_pathEditAct, SIGNAL(triggered()),this,SLOT(togglePath()));

    m_newWindowAct = new QAction(tr("New Window"), this);
    m_newWindowAct->setShortcut(QKeySequence("Ctrl+N"));
    m_newWindowAct->setShortcutContext( Qt::ApplicationShortcut );
    m_newWindowAct->setObjectName("actionNewWindow");
    connect(m_newWindowAct, SIGNAL(triggered()), this, SLOT(newWindow()));
    addAction(m_newWindowAct);
}

void
MainWindow::createMenus()
{
    menuBar()->clear();
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_propertiesAct);
    m_fileMenu->addAction(m_exitAct);

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_mkDirAct);
    m_editMenu->addAction(m_delCurrentSelectionAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_copyAct);
    m_editMenu->addAction(m_cutAct);
    m_editMenu->addAction(m_pasteAct);
    m_editMenu->addAction(m_renameAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_showHiddenAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_refreshAct);
    m_editMenu->addAction(m_addTabAct);
    m_editMenu->addAction(m_openInTabAct);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_configureAct);

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_menuAct);
    m_viewMenu->addAction(m_statAct);
    m_viewMenu->addAction(m_pathEditAct);
    m_viewMenu->addAction(m_pathVisibleAct);

    m_goMenu = menuBar()->addMenu(tr("&Go"));
    m_goMenu->addAction(m_goUpAct);
    m_goMenu->addAction(m_goBackAct);
    m_goMenu->addAction(m_goForwardAct);
    m_goMenu->addSeparator();
    m_goMenu->addAction(m_homeAct);
    m_goMenu->addSeparator();
    foreach ( QAction *action, m_goMenu->actions() )
        if ( action->objectName().isEmpty() )
            delete action;
    for ( int i = 0; i < m_placesView->topLevelItemCount(); ++i )
        m_goMenu->addMenu(m_placesView->containerAsMenu(i));

    menuBar()->addSeparator();

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAct);
    m_helpMenu->addAction(m_aboutQtAct);
    addAction(m_menuAct);
}

void
MainWindow::createToolBars()
{
    m_navToolBar->setMovable(false);
    m_navToolBar->addAction(m_goBackAct);
    m_navToolBar->addAction(m_goForwardAct);

    m_viewToolBar->setMovable(false);
    m_viewToolBar->addWidget(m_toolBarSpacer);
    m_viewToolBar->addAction(m_iconViewAct);
    m_viewToolBar->addAction(m_listViewAct);
    m_viewToolBar->addAction(m_colViewAct);
    m_viewToolBar->addAction(m_flowAct);

//    filterBox->setPlaceholderText("Type to filter...");
    QWidget *spacerMax = new QWidget(m_viewToolBar);
    spacerMax->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    QWidget *spacerMin = new QWidget(m_viewToolBar);
    spacerMin->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    spacerMin->setMaximumWidth(64);
    m_viewToolBar->addWidget(spacerMin);
    m_viewToolBar->addAction(m_configureAct);
    m_viewToolBar->addSeparator();
    m_viewToolBar->addAction(m_homeAct);
    m_viewToolBar->addWidget(spacerMax);
    m_viewToolBar->addWidget(m_filterBox);
}

void
MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu popupMenu;
    QWidget *w = childAt(event->pos());

    QMenu *popup = createPopupMenu();
    popup->addSeparator();
    popup->addAction(m_menuAct);
    popup->addAction(m_statAct);
    popup->addAction(m_pathVisibleAct);

    if(QToolButton *tbn = qobject_cast<QToolButton*>(w))
    {
        popup->exec(mapToGlobal(event->pos()));
        return;
    }

    if(w->parentWidget())
        if(qobject_cast<QToolBar *>(w) || qobject_cast<QToolBar *>(w->parentWidget()))
        {
            popup->exec(mapToGlobal(event->pos()));
            return;
        }
    if(qobject_cast<QStatusBar*>(w))
    {
        popup->exec(mapToGlobal(event->pos()));
        return;
    }

    QMainWindow::contextMenuEvent(event);
}

