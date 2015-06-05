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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "globals.h"

class QSlider;
class QAbstractItemView;
class QModelIndex;
class QItemSelection;
class QToolButton;
class QProgressBar;
class QActionGroup;
class QUrl;
namespace DFM
{
namespace FS{class Model;}
namespace Docks{class DockWidget;}
class TabBar;
class TabManager;
class ViewContainer;
class PlacesView;
class InfoWidget;
class SearchBox;
class StatusBar;
class RecentFoldersView;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(const QStringList &arguments = QStringList(), bool autoTab = true);
    ~MainWindow();
    ViewContainer *activeContainer();
    FS::Model *model();
    inline PlacesView *placesView() { return m_placesView; }
    inline QToolBar *toolBar() { return m_toolBar; }
    static ViewContainer *currentContainer();
    static MainWindow *currentWindow();
    static MainWindow *window(QWidget *w) { QWidget *p = w; while (p->parentWidget()) p = p->parentWidget(); return qobject_cast<MainWindow *>(p); }
    static PlacesView *places();
    static QList<MainWindow *> openWindows();
    ViewContainer *containerForTab(int tab);
    inline InfoWidget *infoWidget() { return m_infoWidget; }
    inline QMenu *mainMenu() { return m_mainMenu; }
    inline QSlider *iconSizeSlider() { return m_iconSizeSlider; }
    inline QAction *action(const Action action) { return m_actions[action]; }
    ViewContainer *takeContainer(int tab);
    SearchBox *searchBox() { return m_filterBox; }
    void updateConfig();
    void createMenus();
    void rightClick(const QString &file, const QPoint &pos);

public slots:
    void addTab(const QUrl &url);
    void addTab(ViewContainer *container, int index = -1);
    void receiveMessage(const QStringList &message);
    void setRootPath(const QString &path);
    void updateFilterBox();

protected:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *ev);
    void contextMenuEvent(QContextMenuEvent *event);
    void keyPressEvent(QKeyEvent *ke);
    void windowActivationChange(bool wasActive);
    void showEvent(QShowEvent *e);
    void createActions();
    void createToolBars();
    void writeSettings();
    void setupStatusBar();
    void connectContainer(ViewContainer *container);

private slots:
    void updateStatusBar(const QUrl &url);
    void updateIcons();
    void goHome();
    void goBack();
    void goUp();
    void goForward();
    void setViewIcons();
    void setViewDetails();
    void setViewCols();
    void filterCurrentDir(const QString &filter);
    void checkViewAct();
    void customCommand();
    void toggleMenuVisible();
    void toggleStatusVisible();
    void about();
    void deleteCurrentSelection();
    void toggleHidden();
    void cutSelection();
    void copySelection();
    void setClipBoard();
    void pasteSelection();
    void rename();
    void flowView();
    void refreshView();
    void genPlace();
    void mainSelectionChanged();
    void togglePath();
    void urlChanged(const QUrl &url);
    void createDirectory();
    void setViewIconSize(int);
    void setSliderPos(int size);
    void currentTabChanged(int tab);
    void setActions();
    void openTab();
    void newTab();
    void viewItemHovered(const QModelIndex &index);
    void viewClearHover();
    void showSettings();
    void fileProperties();
    void hidePath();
    void tabCloseRequest(int);
    void newWindow();
    void readSettings();
    void activateUrl(const QUrl &url);
    void addBookmarks();
    void setSorting();
    void sortingChanged(const int column, const int order);
    void updateToolbarSpacer();
    void trash();

signals:
    void viewChanged(QAbstractItemView *view);
    void settingsChanged();

private:
    StatusBar *m_statusBar;
    QWidget *m_toolBarSpacer;
    SearchBox *m_filterBox;
    Docks::DockWidget *m_placesDock, *m_recentDock, *m_infoDock;
    QString m_statusMessage, m_slctnMessage;
    QItemSelection *currentSelection;
    QSlider *m_iconSizeSlider;
    QLayout *m_statusLayout;
    InfoWidget *m_infoWidget;
    RecentFoldersView *m_recentFoldersView;
    PlacesView *m_placesView;
    TabManager *m_tabManager;
    TabBar *m_tabBar;
    QMainWindow *m_tabWin;
    QMenu *m_mainMenu;
    QToolButton *m_sortButton, *m_menuButton;
    QAction *m_menuAction, *m_menuSep;
    QToolBar *m_toolBar;
    QProgressBar *m_ioProgress;

    QMenu *m_goMenu;
    QAction *m_actions[ActionCount];
    QActionGroup *m_sortActs;

    bool m_cut;
    static MainWindow *s_currentWindow;
    static QList<MainWindow *> s_openWindows;
};

}
#endif
