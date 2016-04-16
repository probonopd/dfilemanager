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


#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include <QFrame>
#include "globals.h"

class QStackedLayout;
class QVBoxLayout;
class QItemSelectionModel;
class QModelIndex;
class QAbstractItemView;
namespace DFM
{
class Button;
class NavBar;
class PathNavigator;
class FlowView;
class ColumnView;
class DetailsView;
class IconView;
namespace FS{class Model;}

class ViewContainer : public QWidget
{
    Q_OBJECT
public:
    enum View { Icon = 0, Details, Column, Flow, NViews };
    static Action viewAction(const View view);

    explicit ViewContainer(QWidget *parent = 0);
    ~ViewContainer();
    FS::Model *model();
    void setView(const View view, bool store = true);
    QAbstractItemView *currentView() const;
    View currentViewType() const { return m_currentView; }
    void setPathEditable(const bool editable);
    void setRootIndex(const QModelIndex &index);
    void createDirectory();
    void setFilter(const QString &filter);
    void deleteCurrentSelection();
    void customCommand();
    const QSize iconSize() const;
    QItemSelectionModel *selectionModel();
    QModelIndex indexAt(const QPoint &p) const;
    bool canGoBack();
    bool canGoForward();
    bool pathVisible();
    void goBack();
    void goForward();
    bool goUp();
    void refresh();
    void rename();
    void goHome();
    void setIconSize(int stop);
    NavBar *breadCrumbs();
    QString currentFilter() const;
    void sort(const int column = 0, const Qt::SortOrder order = Qt::AscendingOrder);
    PathNavigator *pathNav();
#define D_VIEW(_TYPE_, _METHOD_) _TYPE_##View *_METHOD_##View()
    D_VIEW(Icon, icon); D_VIEW(Details, details); D_VIEW(Column, column); D_VIEW(Flow, flow);
#undef D_VIEW

protected:
    void leaveEvent(QEvent *) { emit leftView(); }

signals:
    void viewChanged();
    void iconSizeChanged(const int size);
    void itemHovered(const QString &index);
    void clearHovered();
    void newTabRequest(const QUrl &url);
    void entered(const QModelIndex &index);
    void viewportEntered();
    void leftView();
    void filterChanged();
    void sortingChanged(const int column, const int order);
    void settingsChanged();
    void selectionChanged();
    void urlChanged(const QUrl &url);
    void urlLoaded(const QUrl &url);
    void hiddenVisibilityChanged(const bool visible);

public slots:
    void activate(const QModelIndex &index);
    void setPathVisible(bool visible);

private slots:
    void setUrl(const QUrl &url);
    void genNewTabRequest(const QModelIndex &index);
    void loadedUrl(const QUrl &url);
    void loadSettings();

private:
    bool m_back;
    QString m_dirFilter;
    View m_currentView;
    FS::Model *m_model;
    QStackedLayout *m_viewStack;
    QItemSelectionModel *m_selectModel;
    NavBar *m_navBar;
    QVBoxLayout *m_layout;
    QAbstractItemView *m_view[NViews];
};

}

#endif // VIEWCONTAINER_H
