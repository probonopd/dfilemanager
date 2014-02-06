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

#include <QStackedWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QMenu>

#include "iconview.h"
#include "detailsview.h"
#include "columnswidget.h"
#include "flowview.h"
#include "filesystemmodel.h"
#include "pathnavigator.h"
#include "searchbox.h"

namespace DFM
{
class Button;
class ColumnsWidget;
class IconView;
class DetailsView;
class FlowView;
class BreadCrumbs;
class PathNavigator;
namespace FS{class Model;}
class ViewContainer : public QFrame
{
    Q_OBJECT
public:
    enum View { Icon = 0, Details = 1, Columns = 2, Flow = 3 };
    ViewContainer(QWidget *parent = 0, const QUrl &url = QUrl());
    ~ViewContainer(){}
    static QList<QAction *> rightClickActions();
    FS::Model *model();
    void setView(View view, bool store = true);
    QAbstractItemView *currentView() const;
    QList<QAbstractItemView *> views();
    View currentViewType() const { return m_myView; }
    DetailsView *detailsView() { return m_detailsView; }
    void setPathEditable(bool editable);
    void setRootIndex(const QModelIndex &index);
    void createDirectory();
    void setFilter(QString filter);
    void deleteCurrentSelection();
    void customCommand();
    QSize iconSize();
    QItemSelectionModel *selectionModel();
    QModelIndex indexAt(const QPoint &p) const;
    bool canGoBack();
    bool canGoForward();
    bool pathVisible();
    void addActions(QList<QAction *> actions);
    void goBack();
    void goForward();
    bool goUp();
    void refresh();
    void rename();
    void goHome();
    void animateIconSize(int start, int stop);
    SearchIndicator *searchIndicator() { return m_searchIndicator; }
    BreadCrumbs *breadCrumbs();
    QString currentFilter();
    QList<QUrl> visited() { return m_backList; }
    void sort(const int column = 0, const Qt::SortOrder order = Qt::AscendingOrder);
    PathNavigator *pathNav();
    QString rootPath() const;

protected:
    void leaveEvent(QEvent *) { emit leftView(); }
    void resizeEvent(QResizeEvent *);
    void setModel(QAbstractItemModel *model);
    void setSelectionModel(QItemSelectionModel *selectionModel);
    void scrollToSelection();
    
signals:
    void viewChanged();
    void iconSizeChanged(int size);
    void itemHovered(QString index);
    void clearHovered();
    void newTabRequest(const QUrl &url);
    void entered(const QModelIndex &index);
    void viewportEntered();
    void leftView();
    void filterChanged();
    void sortingChanged(const int column, const int order);
    void urlChanged(const QUrl &url);
    
public slots:
    void activate(const QModelIndex &index);
    bool setPathVisible(bool visible);

private slots:
    void setUrl(const QUrl &url);
    void genNewTabRequest(QModelIndex index);
    void customActionTriggered();
    void scriptTriggered();
    void urlLoaded(const QUrl &url);
    void modelSort(const int column, const int order);

private:
    bool m_back;
    QString m_dirFilter;
    View m_myView;
    IconView *m_iconView;
    DetailsView *m_detailsView;
    ColumnsWidget *m_columnsWidget;
    FlowView *m_flowView;
    FS::Model *m_model;
    QStackedWidget *m_viewStack;
    QList<QUrl> m_backList, m_forwardList;
    QItemSelectionModel *m_selectModel;
    BreadCrumbs *m_breadCrumbs;
    SearchIndicator *m_searchIndicator;
};

}

#endif // VIEWCONTAINER_H
