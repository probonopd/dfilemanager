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
    QColor tbfgc(m_toolBar->palette().color(m_toolBar->foregroundRole()));
    int tbis = m_toolBar->iconSize().height();
    m_delCurrentSelectionAct = new QAction(QIcon::fromTheme("edit-delete"), tr("&Delete"), this);
    //    delCurrentSelectionAct->setShortcuts( QList<QKeySequence>() << QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    m_delCurrentSelectionAct->setShortcut(QKeySequence("Shift+Del"));
    m_delCurrentSelectionAct->setShortcutContext( Qt::ApplicationShortcut );
    m_delCurrentSelectionAct->setObjectName("actionDelete");
    connect(m_delCurrentSelectionAct, SIGNAL(triggered()), this, SLOT(deleteCurrentSelection()));
    addAction(m_delCurrentSelectionAct);

    m_homeAct = new QAction(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Go Home"), this);
    //  homeAct->setStatusTip(tr("Go to ur home dir"));
    m_homeAct->setObjectName("actionHome");
    connect(m_homeAct, SIGNAL(triggered()), this, SLOT(goHome()));
    addAction(m_homeAct);

    m_goBackAct = new QAction( IconProvider::icon(IconProvider::GoBack, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Go Back"), this);
    m_goBackAct->setShortcuts( QKeySequence::Back);
    m_goBackAct->setShortcutContext( Qt::ApplicationShortcut );
    m_goBackAct->setObjectName("actionGoBack");
    //  goBackAct->setStatusTip(tr("Go to previous folder"));
    connect(m_goBackAct, SIGNAL(triggered()), this, SLOT(goBack()));
    addAction(m_goBackAct);

    m_goUpAct = new QAction(QIcon::fromTheme("go-up"), tr("&Go Up"), this);
    m_goUpAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Alt+Up") );
    m_goUpAct->setShortcutContext( Qt::ApplicationShortcut );
    m_goUpAct->setObjectName("actionGoUp");
    connect(m_goUpAct, SIGNAL(triggered()), this, SLOT(goUp()));
    addAction(m_goUpAct);

    m_goForwardAct = new QAction(IconProvider::icon(IconProvider::GoForward, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Go Forward"), this);
    m_goForwardAct->setShortcuts(QKeySequence::Forward);
    m_goForwardAct->setShortcutContext( Qt::ApplicationShortcut );
    m_goForwardAct->setObjectName("actionGoForward");
    //  goForwardAct->setStatusTip(tr("Go to next folder"));
    connect(m_goForwardAct, SIGNAL(triggered()), this, SLOT(goForward()));
    addAction(m_goForwardAct);

    m_exitAct = new QAction(tr("E&xit"), this);
    m_exitAct->setShortcuts(QKeySequence::Quit);
    m_exitAct->setShortcutContext( Qt::ApplicationShortcut );
    m_exitAct->setObjectName("actionExit");
    //  exitAct->setStatusTip(tr("Exit the application"));
    connect(m_exitAct, SIGNAL(triggered()), this, SLOT(close()));
    addAction(m_exitAct);

    m_aboutAct = new QAction(tr("&About"), this);
    m_aboutAct->setObjectName("actionAbout");
    //  aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    addAction(m_aboutAct);

    m_aboutQtAct = new QAction(tr("About &Qt"), this);
    m_aboutQtAct->setObjectName("actionAboutQt");
    //  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(m_aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    addAction(m_aboutQtAct);

    m_placeAct = new QAction(QIcon::fromTheme("bookmark-new"),tr("&Add Place"), this);
//    placeAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+d") );
//    placeAct->setShortcutContext( Qt::ApplicationShortcut );
    m_placeAct->setObjectName("actionPlace");
    connect(m_placeAct, SIGNAL(triggered()), this, SLOT(genPlace()));
    addAction(m_placeAct);

    m_renPlaceAct = new QAction(QIcon::fromTheme("bookmarks-organize"),tr("&Rename Place"), this);
    //  renPlaceAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+d") );
    //  renPlaceAct->setShortcutContext( Qt::ApplicationShortcut );
    m_renPlaceAct->setObjectName("actionRenPlace");
    connect(m_renPlaceAct, SIGNAL(triggered()), m_placesView, SLOT(renPlace()));
    addAction(m_renPlaceAct);

    m_placeContAct = new QAction(QIcon::fromTheme("bookmark-new"),tr("&Add Place Container"), this);
//    placeContAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+Alt+P") );
//    placeContAct->setShortcutContext( Qt::ApplicationShortcut );
    m_placeContAct->setObjectName("actionPlaceContainer");
    connect(m_placeContAct, SIGNAL(triggered()), m_placesView, SLOT(addPlaceCont()));
    addAction(m_placeContAct);

    m_rmPlaceAct = new QAction(QIcon::fromTheme("list-remove"),tr("&Remove Place"), this);
//    rmPlaceAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+Shift+D") );
//    rmPlaceAct->setShortcutContext( Qt::ApplicationShortcut );
    m_rmPlaceAct->setObjectName("actionRmPlace");
    connect(m_rmPlaceAct, SIGNAL(triggered()), m_placesView, SLOT(removePlace()));
    addAction(m_rmPlaceAct);

    m_iconViewAct = new QAction(IconProvider::icon(IconProvider::IconView, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Icons View"), this);
    m_iconViewAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+1") );
    m_iconViewAct->setShortcutContext( Qt::ApplicationShortcut );
    m_iconViewAct->setObjectName("actionIconView");
    m_iconViewAct->setCheckable(true);
    m_iconViewAct->setChecked(true);
    connect(m_iconViewAct, SIGNAL(triggered()), this, SLOT(setViewIcons()));
    addAction(m_iconViewAct);

    m_listViewAct = new QAction(IconProvider::icon(IconProvider::DetailsView, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Details View"), this);
    m_listViewAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+2") );
    m_listViewAct->setShortcutContext( Qt::ApplicationShortcut );
    m_listViewAct->setObjectName("actionsListView");
    m_listViewAct->setCheckable(true);
    connect(m_listViewAct, SIGNAL(triggered()), this, SLOT(setViewDetails()));
    addAction(m_listViewAct);

    m_colViewAct = new QAction(IconProvider::icon(IconProvider::ColumnsView, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Columns View"), this);
    m_colViewAct->setShortcuts( QList<QKeySequence>() << QKeySequence("Ctrl+3") );
    m_colViewAct->setShortcutContext( Qt::ApplicationShortcut );
    m_colViewAct->setObjectName("actionColView");
    m_colViewAct->setCheckable(true);
    connect(m_colViewAct, SIGNAL(triggered()), this, SLOT(setViewCols()));
    addAction(m_colViewAct);

    m_flowAct = new QAction(IconProvider::icon(IconProvider::FlowView, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Flow"), this);
    m_flowAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+4") );
    m_flowAct->setShortcutContext( Qt::ApplicationShortcut );
    m_flowAct->setObjectName("actionFlow");
    m_flowAct->setCheckable(true);
    connect(m_flowAct, SIGNAL(triggered()), this, SLOT(flowView()));
    addAction(m_flowAct);

    m_showHiddenAct = new QAction(tr("&Show Hidden"), this);
    m_showHiddenAct->setShortcuts( QList<QKeySequence>() << QKeySequence("F8") );
    m_showHiddenAct->setShortcutContext( Qt::ApplicationShortcut );
    m_showHiddenAct->setObjectName("actionShowHidden");
    m_showHiddenAct->setCheckable(true);
    connect(m_showHiddenAct, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    addAction(m_showHiddenAct);

    m_mkDirAct = new QAction(QIcon::fromTheme("folder-new"), tr("&Create Directory"), this);
    m_mkDirAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F10") );
    m_mkDirAct->setShortcutContext( Qt::ApplicationShortcut );
    m_mkDirAct->setObjectName("actionMkDir");
    connect(m_mkDirAct, SIGNAL(triggered()), this, SLOT(createDirectory()));
    addAction(m_mkDirAct);

    m_copyAct = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    m_copyAct->setShortcuts(QKeySequence::Copy);
    m_copyAct->setShortcutContext( Qt::ApplicationShortcut );
    m_copyAct->setObjectName("actionCopy");
    connect(m_copyAct, SIGNAL(triggered()), this, SLOT(setClipBoard()));
    addAction(m_copyAct);

    m_cutAct = new QAction(QIcon::fromTheme("edit-cut"), tr("&Cut"), this);
    m_cutAct->setShortcut(QKeySequence("Ctrl+X"));
    m_cutAct->setShortcutContext( Qt::ApplicationShortcut );
    m_cutAct->setObjectName("actionCut");
    connect(m_cutAct, SIGNAL(triggered()), this, SLOT(cutSelection()));
    addAction(m_cutAct);

    m_pasteAct = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    m_pasteAct->setShortcuts(QKeySequence::Paste);
    m_pasteAct->setShortcutContext( Qt::ApplicationShortcut );
    m_pasteAct->setObjectName("actionPaste");
    connect(m_pasteAct, SIGNAL(triggered()), this, SLOT(pasteSelection()));
    addAction(m_pasteAct);

    m_renameAct = new QAction(QIcon::fromTheme("edit-rename"), tr("&Rename"), this);
    m_renameAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F2") );
    m_renameAct->setShortcutContext( Qt::ApplicationShortcut );
    m_renameAct->setObjectName("actionRename");
    connect(m_renameAct, SIGNAL(triggered()), this, SLOT(rename()));
    addAction(m_renameAct);

    m_refreshAct = new QAction(QIcon::fromTheme("view-refresh"), tr("&Refresh"), this);
    m_refreshAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F5") );
    m_refreshAct->setShortcutContext( Qt::ApplicationShortcut );
    m_refreshAct->setObjectName("acgtionRefresh");
    connect(m_refreshAct, SIGNAL(triggered()), this, SLOT(refreshView()));
    addAction(m_refreshAct);

    m_placeIconAct = new QAction(tr("&Gustom Icon"), this);
    m_placeIconAct->setObjectName("actionPlaceIcon");
    connect(m_placeIconAct, SIGNAL(triggered()), m_placesView, SLOT(setPlaceIcon()));
    addAction(m_placeIconAct);

    m_cstCmdAct = new QAction(tr("&Open With Cmd"), this);
    m_cstCmdAct->setObjectName("actionCustomCmd");
    connect(m_cstCmdAct, SIGNAL(triggered()), this, SLOT(customCommand()));
    addAction(m_cstCmdAct);

    m_menuAct = new QAction(tr("Show MenuBar"),this);
    m_menuAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+M") );
    m_menuAct->setShortcutContext( Qt::ApplicationShortcut );
    m_menuAct->setObjectName("actionMenu");
    m_menuAct->setCheckable(true);
    m_menuAct->setEnabled(!Store::config.behaviour.gayWindow);
    connect(m_menuAct,SIGNAL(triggered()),this, SLOT(toggleMenuVisible()));
    addAction(m_menuAct);

    m_statAct = new QAction(tr("Show StatusBar"),this);
    m_statAct->setObjectName("actionStatus");
    m_statAct->setCheckable(true);
    m_statAct->setShortcut( QKeySequence("Ctrl+S") );
    connect(m_statAct,SIGNAL(triggered()),this, SLOT(toggleStatusVisible()));
    addAction(m_statAct);

    m_pathVisibleAct = new QAction(tr("Show Path"),this);
    m_pathVisibleAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+P") );
    m_pathVisibleAct->setShortcutContext( Qt::ApplicationShortcut );
    m_pathVisibleAct->setObjectName("actionPathVisible");
    m_pathVisibleAct->setCheckable(true);
    connect(m_pathVisibleAct, SIGNAL(triggered()),this,SLOT(hidePath()));
    addAction(m_pathVisibleAct);

    m_addTabAct = new QAction(tr("New Tab"),this);
    m_addTabAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+T") );
    m_addTabAct->setShortcutContext( Qt::ApplicationShortcut );
    m_addTabAct->setObjectName("actionAddTab");
    connect(m_addTabAct,SIGNAL(triggered()),this,SLOT(newTab()));
    addAction(m_addTabAct);

    m_openInTabAct = new QAction(tr("Open In New Tab"),this);
    m_openInTabAct->setObjectName("actionOpenInTab");
    connect(m_openInTabAct,SIGNAL(triggered()),this,SLOT(openTab()));
    addAction(m_openInTabAct);

    m_configureAct = new QAction(tr("Configure"),this);
    m_configureAct->setObjectName("configAct");
    m_configureAct->setIcon(IconProvider::icon(IconProvider::Configure, tbis, tbfgc, Store::config.behaviour.systemIcons));
    connect(m_configureAct, SIGNAL(triggered()),this,SLOT(showSettings()));
    addAction(m_configureAct);

    m_propertiesAct = new QAction(tr("Properties"),this);
    m_propertiesAct->setObjectName("propAct");
    m_propertiesAct->setIcon(QIcon::fromTheme("Configure"));
    connect(m_propertiesAct, SIGNAL(triggered()),this,SLOT(fileProperties()));
    addAction(m_propertiesAct);

    m_pathEditAct = new QAction(tr("Edit Path"),this);
    m_pathEditAct->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F6") );
    m_pathEditAct->setShortcutContext( Qt::ApplicationShortcut );
    m_pathEditAct->setObjectName("actionEditPath");
    m_pathEditAct->setCheckable(true);
    connect(m_pathEditAct, SIGNAL(triggered()),this,SLOT(togglePath()));
    addAction(m_pathEditAct);

    m_newWindowAct = new QAction(tr("New Window"), this);
    m_newWindowAct->setShortcut(QKeySequence("Ctrl+N"));
    m_newWindowAct->setShortcutContext( Qt::ApplicationShortcut );
    m_newWindowAct->setObjectName("actionNewWindow");
    connect(m_newWindowAct, SIGNAL(triggered()), this, SLOT(newWindow()));
    addAction(m_newWindowAct);

    m_sortDescAct = new QAction(tr("Descending"), this);
    m_sortDescAct->setCheckable(true);
    connect ( m_sortDescAct, SIGNAL(triggered()), this, SLOT(setSorting()) );

    m_sortActs = new QActionGroup(this);
    m_sortNameAct = m_sortActs->addAction(tr("Name"));
    m_sortDateAct = m_sortActs->addAction(tr("Date"));
    m_sortSizeAct = m_sortActs->addAction(tr("Size"));
    m_sortTypeAct = m_sortActs->addAction(tr("Type"));
    foreach ( QAction *a, m_sortActs->actions() )
    {
        a->setCheckable(true);
        connect ( a, SIGNAL(triggered()), this, SLOT(setSorting()) );
    }
    m_sortActs->setExclusive(true);
}

void
MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_propertiesAct);
    m_fileMenu->addAction(m_exitAct);

    m_mainMenu = new QMenu("Menu", this);
    m_mainMenu->addMenu(m_fileMenu);

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
    m_mainMenu->addMenu(m_editMenu);

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_menuAct);
    m_viewMenu->addAction(m_statAct);
    m_viewMenu->addAction(m_pathEditAct);
    m_viewMenu->addAction(m_pathVisibleAct);
    m_mainMenu->addMenu(m_viewMenu);

    m_goMenu = new Menu(this);
    m_goMenu->setTitle(tr("&Go"));
    menuBar()->addMenu(m_goMenu);
    m_goMenu->addAction(m_goUpAct);
    m_goMenu->addAction(m_goBackAct);
    m_goMenu->addAction(m_goForwardAct);
    m_goMenu->addSeparator();
    m_goMenu->addAction(m_homeAct);
    m_goMenu->addSeparator();
    connect ( m_goMenu, SIGNAL(aboutToShow()), this, SLOT(addBookmarks()) );
    m_mainMenu->addMenu(m_goMenu);

    menuBar()->addSeparator();

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAct);
    m_helpMenu->addAction(m_aboutQtAct);
    addAction(m_menuAct);
    m_mainMenu->addMenu(m_helpMenu);

    if ( Store::config.behaviour.gayWindow )
        m_menuAct->setChecked(false);
}

void
MainWindow::addBookmarks()
{
    foreach ( QAction *action, m_goMenu->actions() )
        if ( action->objectName().isEmpty() && !action->isSeparator() )
            delete action;
    for ( int i = 0; i < m_placesView->containerCount(); ++i )
        m_goMenu->addMenu(m_placesView->containerAsMenu(i));
}

void
MainWindow::createToolBars()
{
    m_toolBar->setMovable(false);
    m_toolBar->addAction(m_goBackAct);
    m_toolBar->addAction(m_goForwardAct);
    m_toolBar->addSeparator();
    m_toolBar->addWidget(m_toolBarSpacer);
    m_toolBar->addAction(m_iconViewAct);
    m_toolBar->addAction(m_listViewAct);
    m_toolBar->addAction(m_colViewAct);
    m_toolBar->addAction(m_flowAct);
    m_toolBar->addSeparator();

    QWidget *spacerFixed = new QWidget(m_toolBar);
    spacerFixed->setFixedWidth(64);
    m_toolBar->addWidget(spacerFixed);
    m_toolBar->addAction(m_configureAct);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_homeAct);
    m_toolBar->addSeparator();
    m_sortButton = new QToolButton(this);
    m_sortButton->setIcon(IconProvider::icon(IconProvider::Sort, 16, m_sortButton->palette().color(m_sortButton->foregroundRole()), false));
    QMenu *sortMenu = new QMenu(m_sortButton);
    sortMenu->setSeparatorsCollapsible(false);
    sortMenu->addSeparator()->setText(tr("Sort By:"));
    sortMenu->addActions(m_sortActs->actions());
    sortMenu->addSeparator();
    sortMenu->addAction(m_sortDescAct);
    m_sortButton->setMenu(sortMenu);
    m_sortButton->setPopupMode(QToolButton::InstantPopup);
    m_toolBar->addWidget(m_sortButton);

    QWidget *searchWidget = new QWidget(m_toolBar);
    searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QGridLayout *gl = new QGridLayout(searchWidget);

    QWidget *spacerMax = new QWidget(searchWidget);
    spacerMax->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    gl->setSpacing(0);
    gl->setContentsMargins(0, 0, 0, 0);
    gl->setColumnStretch(1, 1);
    gl->addWidget(spacerMax, 0, 0);
    gl->addWidget(m_filterBox, 0, 1);

    m_toolBar->addWidget(searchWidget);
    QTimer::singleShot(1000, this, SLOT(updateIcons()));
}

void
MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QWidget *w = childAt(event->pos());

    QMenu *popup = createPopupMenu();
    popup->addSeparator();
    popup->addAction(m_menuAct);
    popup->addAction(m_statAct);
    popup->addAction(m_pathVisibleAct);

    if (qobject_cast<QToolButton*>(w))
    {
        popup->exec(mapToGlobal(event->pos()));
        return;
    }

    if (w->parentWidget())
        if (qobject_cast<QToolBar *>(w) || qobject_cast<QToolBar *>(w->parentWidget()))
        {
            popup->exec(mapToGlobal(event->pos()));
            return;
        }
    if (qobject_cast<QStatusBar *>(w))
    {
        popup->exec(mapToGlobal(event->pos()));
        return;
    }

    QMainWindow::contextMenuEvent(event);
}

