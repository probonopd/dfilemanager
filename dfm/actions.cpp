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

#include <QMenuBar>
#include <QButtonGroup>
#include <QToolBar>
#include <QContextMenuEvent>
#include <QStatusBar>
#include <QToolButton>
#include <QAction>
#include <QApplication>

#include "mainwindow.h"
#include "iconprovider.h"
#include "config.h"
#include "placesview.h"
#include "searchbox.h"
#include "viewcontainer.h"
#include "fsmodel.h"

using namespace DFM;

void
MainWindow::createActions()
{
    QColor tbfgc(m_toolBar->palette().color(m_toolBar->foregroundRole()));
    int tbis = m_toolBar->iconSize().height();
    m_actions[DeleteSelection] = new QAction(QIcon::fromTheme("edit-delete"), tr("&Delete"), this);
    //    delCurrentSelectionAct->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::SHIFT + Qt::Key_Delete));
    m_actions[DeleteSelection]->setShortcut(QKeySequence("Shift+Del"));
    m_actions[DeleteSelection]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[DeleteSelection]->setObjectName("actionDelete");
    connect(m_actions[DeleteSelection], SIGNAL(triggered()), this, SLOT(deleteCurrentSelection()));
    addAction(m_actions[DeleteSelection]);

    m_actions[GoHome] = new QAction(IconProvider::icon(IconProvider::GoHome, tbis, tbfgc, Store::config.behaviour.systemIcons), tr("&Go Home"), this);
    //  homeAct->setStatusTip(tr("Go to ur home dir"));
    m_actions[GoHome]->setObjectName("actionHome");
    connect(m_actions[GoHome], SIGNAL(triggered()), this, SLOT(goHome()));
    addAction(m_actions[GoHome]);

    m_actions[GoBack] = new QAction(tr("&Go Back"), this);
    m_actions[GoBack]->setShortcuts(QKeySequence::Back);
    m_actions[GoBack]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[GoBack]->setObjectName("actionGoBack");
    //  goBackAct->setStatusTip(tr("Go to previous folder"));
    connect(m_actions[GoBack], SIGNAL(triggered()), this, SLOT(goBack()));
    addAction(m_actions[GoBack]);

    m_actions[GoUp] = new QAction(QIcon::fromTheme("go-up"), tr("&Go Up"), this);
    m_actions[GoUp]->setShortcuts(QList<QKeySequence>() << QKeySequence("Alt+Up"));
    m_actions[GoUp]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[GoUp]->setObjectName("actionGoUp");
    connect(m_actions[GoUp], SIGNAL(triggered()), this, SLOT(goUp()));
    addAction(m_actions[GoUp]);

    m_actions[GoForward] = new QAction(tr("&Go Forward"), this);
    m_actions[GoForward]->setShortcuts(QKeySequence::Forward);
    m_actions[GoForward]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[GoForward]->setObjectName("actionGoForward");
    //  goForwardAct->setStatusTip(tr("Go to next folder"));
    connect(m_actions[GoForward], SIGNAL(triggered()), this, SLOT(goForward()));
    addAction(m_actions[GoForward]);

    m_actions[Exit] = new QAction(tr("E&xit"), this);
    m_actions[Exit]->setShortcuts(QKeySequence::Quit);
    m_actions[Exit]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Exit]->setObjectName("actionExit");
    //  exitAct->setStatusTip(tr("Exit the application"));
    connect(m_actions[Exit], SIGNAL(triggered()), this, SLOT(close()));
    addAction(m_actions[Exit]);

    m_actions[About] = new QAction(tr("&About"), this);
    m_actions[About]->setObjectName("actionAbout");
    //  aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_actions[About], SIGNAL(triggered()), this, SLOT(about()));
    addAction(m_actions[About]);

    m_actions[AboutQt] = new QAction(tr("About &Qt"), this);
    m_actions[AboutQt]->setObjectName("actionAboutQt");
    //  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(m_actions[AboutQt], SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    addAction(m_actions[AboutQt]);

    m_actions[AddPlace] = new QAction(QIcon::fromTheme("bookmark-new"),tr("&Add Place"), this);
//    placeAct->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+d"));
//    placeAct->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[AddPlace]->setObjectName("actionPlace");
    connect(m_actions[AddPlace], SIGNAL(triggered()), this, SLOT(genPlace()));
    addAction(m_actions[AddPlace]);

    m_actions[RenamePlace] = new QAction(QIcon::fromTheme("bookmarks-organize"),tr("&Rename Place"), this);
    //  renPlaceAct->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+d"));
    //  renPlaceAct->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[RenamePlace]->setObjectName("actionRenPlace");
    connect(m_actions[RenamePlace], SIGNAL(triggered()), m_placesView, SLOT(renPlace()));
    addAction(m_actions[RenamePlace]);

    m_actions[AddPlaceContainer] = new QAction(QIcon::fromTheme("bookmark-new"),tr("&Add Place Container"), this);
//    placeContAct->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+Alt+P"));
//    placeContAct->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[AddPlaceContainer]->setObjectName("actionPlaceContainer");
    connect(m_actions[AddPlaceContainer], SIGNAL(triggered()), m_placesView, SLOT(addPlaceCont()));
    addAction(m_actions[AddPlaceContainer]);

    m_actions[RemovePlace] = new QAction(QIcon::fromTheme("list-remove"),tr("&Remove Place"), this);
//    rmPlaceAct->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+Shift+D"));
//    rmPlaceAct->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[RemovePlace]->setObjectName("actionRmPlace");
    connect(m_actions[RemovePlace], SIGNAL(triggered()), m_placesView, SLOT(removePlace()));
    addAction(m_actions[RemovePlace]);

    QActionGroup *viewActs = new QActionGroup(this);
    viewActs->setExclusive(true);
    m_actions[Views_Icon] = viewActs->addAction(new QAction(tr("&Icons View"), this));
    m_actions[Views_Icon]->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+1"));
    m_actions[Views_Icon]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Views_Icon]->setObjectName("actionIconView");
    m_actions[Views_Icon]->setCheckable(true);
    m_actions[Views_Icon]->setChecked(true);
    connect(m_actions[Views_Icon], SIGNAL(triggered()), this, SLOT(setViewIcons()));
    addAction(m_actions[Views_Icon]);

    m_actions[Views_Detail] = viewActs->addAction(new QAction(tr("&Details View"), this));
    m_actions[Views_Detail]->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+2"));
    m_actions[Views_Detail]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Views_Detail]->setObjectName("actionsListView");
    m_actions[Views_Detail]->setCheckable(true);
    connect(m_actions[Views_Detail], SIGNAL(triggered()), this, SLOT(setViewDetails()));
    addAction(m_actions[Views_Detail]);

    m_actions[Views_Column] = viewActs->addAction(new QAction(tr("&Columns View"), this));
    m_actions[Views_Column]->setShortcuts(QList<QKeySequence>() << QKeySequence("Ctrl+3"));
    m_actions[Views_Column]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Views_Column]->setObjectName("actionColView");
    m_actions[Views_Column]->setCheckable(true);
    connect(m_actions[Views_Column], SIGNAL(triggered()), this, SLOT(setViewCols()));
    addAction(m_actions[Views_Column]);

    m_actions[Views_Flow] = viewActs->addAction(new QAction(tr("&Flow"), this));
    m_actions[Views_Flow]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+4"));
    m_actions[Views_Flow]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Views_Flow]->setObjectName("actionFlow");
    m_actions[Views_Flow]->setCheckable(true);
    connect(m_actions[Views_Flow], SIGNAL(triggered()), this, SLOT(flowView()));
    addAction(m_actions[Views_Flow]);

    m_actions[ShowHidden] = new QAction(tr("&Show Hidden"), this);
    m_actions[ShowHidden]->setShortcuts(QList<QKeySequence>() << QKeySequence("F8"));
    m_actions[ShowHidden]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[ShowHidden]->setObjectName("actionShowHidden");
    m_actions[ShowHidden]->setCheckable(true);
    connect(m_actions[ShowHidden], SIGNAL(triggered()), this, SLOT(toggleHidden()));
    addAction(m_actions[ShowHidden]);

    m_actions[MkDir] = new QAction(QIcon::fromTheme("folder-new"), tr("&Create Directory"), this);
    m_actions[MkDir]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F10"));
    m_actions[MkDir]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[MkDir]->setObjectName("actionMkDir");
    connect(m_actions[MkDir], SIGNAL(triggered()), this, SLOT(createDirectory()));
    addAction(m_actions[MkDir]);

    m_actions[Copy] = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    m_actions[Copy]->setShortcuts(QKeySequence::Copy);
    m_actions[Copy]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Copy]->setObjectName("actionCopy");
    connect(m_actions[Copy], SIGNAL(triggered()), this, SLOT(setClipBoard()));
    addAction(m_actions[Copy]);

    m_actions[Cut] = new QAction(QIcon::fromTheme("edit-cut"), tr("&Cut"), this);
    m_actions[Cut]->setShortcut(QKeySequence("Ctrl+X"));
    m_actions[Cut]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Cut]->setObjectName("actionCut");
    connect(m_actions[Cut], SIGNAL(triggered()), this, SLOT(cutSelection()));
    addAction(m_actions[Cut]);

    m_actions[Paste] = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    m_actions[Paste]->setShortcuts(QKeySequence::Paste);
    m_actions[Paste]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Paste]->setObjectName("actionPaste");
    connect(m_actions[Paste], SIGNAL(triggered()), this, SLOT(pasteSelection()));
    addAction(m_actions[Paste]);

    m_actions[Rename] = new QAction(QIcon::fromTheme("edit-rename"), tr("&Rename"), this);
    m_actions[Rename]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F2"));
    m_actions[Rename]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Rename]->setObjectName("actionRename");
    connect(m_actions[Rename], SIGNAL(triggered()), this, SLOT(rename()));
    addAction(m_actions[Rename]);

    m_actions[Refresh] = new QAction(QIcon::fromTheme("view-refresh"), tr("&Refresh"), this);
    m_actions[Refresh]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F5"));
    m_actions[Refresh]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[Refresh]->setObjectName("acgtionRefresh");
    connect(m_actions[Refresh], SIGNAL(triggered()), this, SLOT(refreshView()));
    addAction(m_actions[Refresh]);

    m_actions[SetPlaceIcon] = new QAction(tr("&Gustom Icon"), this);
    m_actions[SetPlaceIcon]->setObjectName("actionPlaceIcon");
    connect(m_actions[SetPlaceIcon], SIGNAL(triggered()), m_placesView, SLOT(setPlaceIcon()));
    addAction(m_actions[SetPlaceIcon]);

    m_actions[CustomCommand] = new QAction(tr("&Custom Command..."), this);
    m_actions[CustomCommand]->setObjectName("actionCustomCmd");
    connect(m_actions[CustomCommand], SIGNAL(triggered()), this, SLOT(customCommand()));
    addAction(m_actions[CustomCommand]);

    m_actions[ShowToolBar] = new QAction(tr("Show ToolBar"),this);
    m_actions[ShowToolBar]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+B"));
    m_actions[ShowToolBar]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[ShowToolBar]->setObjectName("actionTool");
    m_actions[ShowToolBar]->setCheckable(true);
    connect(m_actions[ShowToolBar],SIGNAL(triggered()),this, SLOT(toggleMenuVisible()));
    addAction(m_actions[ShowToolBar]);

    m_actions[ShowMenuBar] = new QAction(tr("Show MenuBar"),this);
    m_actions[ShowMenuBar]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+M"));
    m_actions[ShowMenuBar]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[ShowMenuBar]->setObjectName("actionMenu");
    m_actions[ShowMenuBar]->setCheckable(true);
    m_actions[ShowMenuBar]->setEnabled(!Store::config.behaviour.gayWindow);
    connect(m_actions[ShowMenuBar],SIGNAL(triggered()),this, SLOT(toggleMenuVisible()));
    addAction(m_actions[ShowMenuBar]);

    m_actions[ShowStatusBar] = new QAction(tr("Show StatusBar"),this);
    m_actions[ShowStatusBar]->setObjectName("actionStatus");
    m_actions[ShowStatusBar]->setCheckable(true);
    m_actions[ShowStatusBar]->setShortcut(QKeySequence("Ctrl+S"));
    connect(m_actions[ShowStatusBar],SIGNAL(triggered()),this, SLOT(toggleStatusVisible()));
    addAction(m_actions[ShowStatusBar]);

    m_actions[ShowPathBar] = new QAction(tr("Show Path"),this);
    m_actions[ShowPathBar]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+P"));
    m_actions[ShowPathBar]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[ShowPathBar]->setObjectName("actionPathVisible");
    m_actions[ShowPathBar]->setCheckable(true);
    connect(m_actions[ShowPathBar], SIGNAL(triggered()),this,SLOT(hidePath()));
    addAction(m_actions[ShowPathBar]);

    m_actions[AddTab] = new QAction(tr("New Tab"),this);
    m_actions[AddTab]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("Ctrl+T"));
    m_actions[AddTab]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[AddTab]->setObjectName("actionAddTab");
    connect(m_actions[AddTab],SIGNAL(triggered()),this,SLOT(newTab()));
    addAction(m_actions[AddTab]);

    m_actions[OpenInTab] = new QAction(tr("Open In New Tab"),this);
    m_actions[OpenInTab]->setObjectName("actionOpenInTab");
    connect(m_actions[OpenInTab],SIGNAL(triggered()),this,SLOT(openTab()));
    addAction(m_actions[OpenInTab]);

    m_actions[Configure] = new QAction(tr("Configure"),this);
    m_actions[Configure]->setObjectName("configAct");
//    m_actions[Configure]->setIcon(IconProvider::icon(IconProvider::Configure, tbis, tbfgc, Store::config.behaviour.systemIcons));
    connect(m_actions[Configure], SIGNAL(triggered()),this,SLOT(showSettings()));
    addAction(m_actions[Configure]);

    m_actions[Properties] = new QAction(tr("Properties"),this);
    m_actions[Properties]->setObjectName("propAct");
//    m_actions[Properties]->setIcon(QIcon::fromTheme("Configure"));
    connect(m_actions[Properties], SIGNAL(triggered()),this,SLOT(fileProperties()));
    addAction(m_actions[Properties]);

    m_actions[EditPath] = new QAction(tr("Edit Path"),this);
    m_actions[EditPath]->setShortcuts(QList<QKeySequence>() <<  QKeySequence("F6"));
    m_actions[EditPath]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[EditPath]->setObjectName("actionEditPath");
    m_actions[EditPath]->setCheckable(true);
    connect(m_actions[EditPath], SIGNAL(triggered()),this,SLOT(togglePath()));
    addAction(m_actions[EditPath]);

    m_actions[NewWindow] = new QAction(tr("New Window"), this);
    m_actions[NewWindow]->setShortcut(QKeySequence("Ctrl+N"));
    m_actions[NewWindow]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[NewWindow]->setObjectName("actionNewWindow");
    connect(m_actions[NewWindow], SIGNAL(triggered()), this, SLOT(newWindow()));
    addAction(m_actions[NewWindow]);

    m_actions[SortDescending] = new QAction(tr("Descending"), this);
    m_actions[SortDescending]->setCheckable(true);
    connect (m_actions[SortDescending], SIGNAL(triggered()), this, SLOT(setSorting()));

    m_sortActs = new QActionGroup(this);
    m_actions[SortName] = m_sortActs->addAction(tr("Name"));
    m_actions[SortDate] = m_sortActs->addAction(tr("Date"));
    m_actions[SortSize] = m_sortActs->addAction(tr("Size"));
    m_actions[SortType] = m_sortActs->addAction(tr("Type"));
    foreach (QAction *a, m_sortActs->actions())
    {
        a->setCheckable(true);
        connect (a, SIGNAL(triggered()), this, SLOT(setSorting()));
    }
    m_sortActs->setExclusive(true);

    m_actions[GetFileName] = new QAction(tr("Copy FileName(s) to ClipBoard"), this);
    m_actions[GetFileName]->setShortcut(QKeySequence("Ctrl+Shift+N"));
    m_actions[GetFileName]->setShortcutContext(Qt::ApplicationShortcut);
    addAction(m_actions[GetFileName]);
    connect(m_actions[GetFileName], SIGNAL(triggered()), Ops::instance(), SLOT(getPathToClipBoard()));

    m_actions[GetFilePath] = new QAction(tr("Copy FilePath(s) to ClipBoard"), this);
    m_actions[GetFilePath]->setShortcut(QKeySequence("Ctrl+Shift+P"));
    m_actions[GetFilePath]->setShortcutContext(Qt::ApplicationShortcut);
    addAction(m_actions[GetFilePath]);
    connect(m_actions[GetFilePath], SIGNAL(triggered()), Ops::instance(), SLOT(getPathToClipBoard()));

    m_actions[MoveToTrashAction] = new QAction(tr("Move Selected File(s) To trash"), this);
    m_actions[MoveToTrashAction]->setShortcut(QKeySequence("Del"));
    m_actions[MoveToTrashAction]->setShortcutContext(Qt::ApplicationShortcut);
    m_actions[MoveToTrashAction]->setObjectName("actionMoveToTrash");
    addAction(m_actions[MoveToTrashAction]);
    connect(m_actions[MoveToTrashAction], SIGNAL(triggered()), this, SLOT(trash()));

    m_actions[RestoreFromTrashAction] = new QAction(tr("Restore"), this);
    m_actions[RestoreFromTrashAction]->setObjectName("actionRestoreFromTrash");
    addAction(m_actions[RestoreFromTrashAction]);
    connect(m_actions[RestoreFromTrashAction], SIGNAL(triggered()), this, SLOT(trash()));

    m_actions[RemoveFromTrashAction] = new QAction(tr("Remove From Trash"), this);
    m_actions[RemoveFromTrashAction]->setObjectName("actionRemoveFromTrash");
    addAction(m_actions[RemoveFromTrashAction]);
    connect(m_actions[RemoveFromTrashAction], SIGNAL(triggered()), this, SLOT(trash()));
}

void
MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_actions[Properties]);
    fileMenu->addAction(m_actions[Exit]);

    m_mainMenu = new QMenu("Menu", this);
    m_mainMenu->addMenu(fileMenu);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_actions[MkDir]);
    editMenu->addAction(m_actions[DeleteSelection]);
    editMenu->addSeparator();
    editMenu->addAction(m_actions[Copy]);
    editMenu->addAction(m_actions[Cut]);
    editMenu->addAction(m_actions[Paste]);
    editMenu->addAction(m_actions[Rename]);
    editMenu->addSeparator();
    editMenu->addAction(m_actions[ShowHidden]);
    editMenu->addSeparator();
    editMenu->addAction(m_actions[Refresh]);
    editMenu->addAction(m_actions[AddTab]);
    editMenu->addAction(m_actions[OpenInTab]);
    editMenu->addSeparator();
    editMenu->addAction(m_actions[Configure]);
    m_mainMenu->addMenu(editMenu);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_actions[ShowMenuBar]);
    viewMenu->addAction(m_actions[ShowToolBar]);
    viewMenu->addAction(m_actions[ShowStatusBar]);
    viewMenu->addAction(m_actions[EditPath]);
    viewMenu->addAction(m_actions[ShowPathBar]);
    m_mainMenu->addMenu(viewMenu);

    m_goMenu = menuBar()->addMenu(tr("&Go"));
    m_goMenu->addAction(m_actions[GoUp]);
    m_goMenu->addAction(m_actions[GoBack]);
    m_goMenu->addAction(m_actions[GoForward]);
    m_goMenu->addSeparator();
    m_goMenu->addAction(m_actions[GoHome]);
    m_goMenu->addSeparator();
    connect (m_goMenu, SIGNAL(aboutToShow()), this, SLOT(addBookmarks()));
    m_mainMenu->addMenu(m_goMenu);

    QMenu *getMenu = menuBar()->addMenu(tr("&Get"));
    getMenu->addAction(m_actions[GetFileName]);
    getMenu->addAction(m_actions[GetFilePath]);
    m_mainMenu->addMenu(getMenu);

    menuBar()->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_actions[About]);
    helpMenu->addAction(m_actions[AboutQt]);
    addAction(m_actions[ShowMenuBar]);
    m_mainMenu->addMenu(helpMenu);

    if (Store::config.behaviour.gayWindow)
        m_actions[ShowMenuBar]->setChecked(false);
}

static QAction *genSeparator(QObject *parent)
{
    QAction *a = new QAction(parent);
    a->setSeparator(true);
    return a;
}

void
MainWindow::rightClick(const QString &file, const QPoint &pos)
{
    if (ViewContainer *vc = activeContainer())
    {
        QMenu menu;
        if (vc->model()->url(vc->currentView()->rootIndex()).scheme().toLower() == "trash")
        {
            menu.addActions(QList<QAction *>() << m_actions[RestoreFromTrashAction] << m_actions[RemoveFromTrashAction]);
        }
        else
        {

            QList<QAction *> firstGroup;
            firstGroup << m_actions[OpenInTab]
                       << m_actions[MkDir]
                       << genSeparator(&menu)
                       << m_actions[Paste]
                       << m_actions[Copy]
                       << m_actions[Cut]
                       << genSeparator(&menu)
                       << m_actions[MoveToTrashAction]
                       << m_actions[DeleteSelection]
                       << m_actions[Rename];
            QList<QAction *> secondGroup;
            secondGroup << m_actions[GoUp]
                        << m_actions[GoBack]
                        << m_actions[GoForward]
                        << genSeparator(&menu)
                        << m_actions[Properties];

            QMenu *openWith = new QMenu("Open With", &menu);
            QList<QAction *> owa = QList<QAction *>() << Store::openWithActions(file) << m_actions[CustomCommand];
            openWith->addActions(owa);

            if (!Store::customActions().isEmpty())
            {
                menu.addMenu(Store::customActionsMenu());
                menu.addSeparator();
            }
            menu.addActions(firstGroup);
            menu.addSeparator();
            menu.addMenu(openWith);
            menu.addSeparator();
            menu.addActions(secondGroup);

        }
        menu.exec(pos);
    }
}

void
MainWindow::addBookmarks()
{
    foreach (QAction *action, m_goMenu->actions())
        if (action->objectName().isEmpty() && !action->isSeparator())
            delete action;
    for (int i = 0; i < m_placesView->containerCount(); ++i)
        m_goMenu->addMenu(m_placesView->containerAsMenu(i));
}

void
MainWindow::createToolBars()
{
    m_toolBar->setMovable(false);
//    if (style()->objectName() == "oxygen")
//    {
//        QWidget *history(new QWidget());
//        history->setContentsMargins(0, 0, 0, 0);
//        QHBoxLayout *hb(new QHBoxLayout());
//        hb->setContentsMargins(0, 0, 0, 0);
//        hb->setSpacing(0);
//        QToolButton *goBack(new QToolButton(history));
//        goBack->setDefaultAction(m_actions[GoBack]);
//        goBack->setCheckable(true);
//        QButtonGroup *g = new QButtonGroup(history);
//        g->addButton(goBack);
//        QToolButton *goForward(new QToolButton(history));
//        goForward->setDefaultAction(m_actions[GoForward]);
//        goForward->setCheckable(true);
//        g->addButton(goForward);
//        hb->addWidget(goBack);
//        hb->addWidget(goForward);
//        history->setLayout(hb);
//        m_toolBar->addWidget(history);
//    }
//    else
//    {
        m_toolBar->addAction(m_actions[GoBack]);
        m_toolBar->addAction(m_actions[GoForward]);
//    }
//    m_toolBar->addSeparator();
    m_toolBar->addWidget(m_toolBarSpacer);
    m_toolBar->addAction(m_actions[Views_Icon]);
    m_toolBar->addAction(m_actions[Views_Detail]);
    m_toolBar->addAction(m_actions[Views_Column]);
    m_toolBar->addAction(m_actions[Views_Flow]);
//    m_toolBar->addSeparator();

//    QWidget *spacerFixed = new QWidget(m_toolBar);
//    spacerFixed->setFixedWidth(64);
//    m_toolBar->addWidget(spacerFixed);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actions[Configure]);

    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actions[GoHome]);

    m_toolBar->addSeparator();
    m_sortButton->setIcon(IconProvider::icon(IconProvider::Sort, 16, m_sortButton->palette().color(m_sortButton->foregroundRole()), false));
    QMenu *sortMenu = new QMenu(m_sortButton);
    sortMenu->setSeparatorsCollapsible(false);
    sortMenu->addSeparator()->setText(tr("Sort By:"));
    sortMenu->addActions(m_sortActs->actions());
    sortMenu->addSeparator();
    sortMenu->addAction(m_actions[SortDescending]);
    m_sortButton->setMenu(sortMenu);
    m_sortButton->setPopupMode(QToolButton::InstantPopup);
    m_toolBar->addWidget(m_sortButton);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actions[ShowHidden]);

    QWidget *stretch = new QWidget(m_toolBar);
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_toolBar->addWidget(stretch);
    m_toolBar->addWidget(m_filterBox);
    m_menuSep = m_toolBar->addSeparator();
    m_menuSep->setVisible(!Store::config.behaviour.gayWindow&&!menuBar()->isVisible());

    m_menuButton->setIcon(IconProvider::icon(IconProvider::Sort, 16, m_sortButton->palette().color(m_sortButton->foregroundRole()), false));
    m_menuButton->setMenu(mainMenu());
    m_menuButton->setPopupMode(QToolButton::InstantPopup);
    m_menuAction = m_toolBar->addWidget(m_menuButton);
    m_menuButton->setVisible(!Store::config.behaviour.gayWindow&&!menuBar()->isVisible());
}

void
MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QWidget *w = childAt(event->pos());

    QMenu *popup = createPopupMenu();
    popup->addSeparator();
    popup->addAction(m_actions[ShowMenuBar]);
    popup->addAction(m_actions[ShowStatusBar]);
    popup->addAction(m_actions[ShowPathBar]);

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

