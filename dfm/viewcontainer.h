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

class QColumnView;
class QStackedWidget;
class QVBoxLayout;
class QItemSelectionModel;
class QModelIndex;
class QAbstractItemView;
namespace DFM
{
class Button;
class IconView;
class DetailsView;
class FlowView;
class NavBar;
class PathNavigator;
class ColumnView;
namespace FS{class Model;}

class ViewContainer : public QFrame
{
    Q_OBJECT
public:
    enum View { Icon = 0, Details = 1, Columns = 2, Flow = 3 };
    static Actions viewAction(const View view);

    explicit ViewContainer(QWidget *parent = 0);
    ~ViewContainer();
    FS::Model *model();
    void setView(const View view, bool store = true);
    QAbstractItemView *currentView() const;
    QList<QAbstractItemView *> views();
    View currentViewType() const { return m_myView; }
    DetailsView *detailsView() { return m_detailsView; }
    void setPathEditable(const bool editable);
    void setRootIndex(const QModelIndex &index);
    void createDirectory();
    void setFilter(const QString &filter);
    void deleteCurrentSelection();
    void customCommand();
    QSize iconSize();
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
    void animateIconSize(int start, int stop);
    NavBar *breadCrumbs();
    QString currentFilter() const;
    void sort(const int column = 0, const Qt::SortOrder order = Qt::AscendingOrder);
    PathNavigator *pathNav();

protected:
    void leaveEvent(QEvent *) { emit leftView(); }
    void setModel(FS::Model *model);
    void setSelectionModel(QItemSelectionModel *selectionModel);

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
    View m_myView;
    IconView *m_iconView;
    DetailsView *m_detailsView;
    ColumnView *m_columnView;
    FlowView *m_flowView;
    FS::Model *m_model;
    QStackedWidget *m_viewStack;
    QItemSelectionModel *m_selectModel;
    NavBar *m_navBar;
    QVBoxLayout *m_layout;
    QAbstractItemView *m_currentView;
};

}

#endif // VIEWCONTAINER_H
