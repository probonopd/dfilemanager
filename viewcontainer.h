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

#include "iconview.h"
#include "detailsview.h"
#include "columnsview.h"
#include "flowview.h"
#include "filesystemmodel.h"
#include "pathnavigator.h"

namespace DFM
{

#define VIEWS(RUN)\
    m_iconView->RUN;\
    m_detailsView->RUN;\
    m_columnsView->RUN;\
    m_flowView->RUN

class ViewContainer : public QFrame
{
    Q_OBJECT
public:
    enum View { Icon = 0, Details = 1, Columns = 2, Flow = 3 };
    ViewContainer(QWidget *parent = 0, QString rootPath = QDir::homePath());
    FileSystemModel *model() const;
    void setView(View view);
    inline QAbstractItemView *currentView() const { if ( m_myView != Flow ) return static_cast<QAbstractItemView*>(m_viewStack->currentWidget()); return static_cast<QAbstractItemView*>(m_flowView->detailsView()); }
    View currentViewType() const { return m_myView; }
    void setPathEditable(bool editable);
    inline void setRootIndex(const QModelIndex &index) { VIEWS(setRootIndex(index)); }
    void createDirectory();
    void setFilter(QString filter);
    void deleteCurrentSelection();
    void customCommand();
    QSize iconSize();
    QItemSelectionModel *selectionModel() const;
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
    BreadCrumbs *breadCrumbs();
    QString currentFilter();

protected:
    inline virtual void leaveEvent(QEvent *) { emit leftView(); }
    
signals:
    void viewChanged();
    void currentPathChanged(QString path);
    void iconSizeChanged(int size);
    void itemHovered(QString index);
    void clearHovered();
    void newTabRequest(QString path);
    void entered(const QModelIndex &index);
    void viewportEntered();
    void leftView();
    
public slots:
    void doubleClick(const QModelIndex &index);
    bool setPathVisible(bool visible);
    void setRootPath(QString rootPath);

private slots:
    void directoryLoaded(QString index);
    void rootPathChanged(QString index);
    void genNewTabRequest(QModelIndex index);
    void customActionTriggered();
    void scriptTriggered();

private:
    void setModel(FileSystemModel *model);
    void setSelectionModel(QItemSelectionModel *selectionModel);
    void scrollToSelection();
    bool m_back;
    QString m_dirFilter;
    View m_myView;
    IconView *m_iconView;
    DetailsView *m_detailsView;
    ColumnsView *m_columnsView;
    FlowView *m_flowView;
    FileSystemModel *m_fsModel;
    QStackedWidget *m_viewStack;
    QStringList m_backList;
    QStringList m_forwardList;
    QItemSelectionModel *m_selectModel;
    BreadCrumbs *m_breadCrumbs;
};

}

#endif // VIEWCONTAINER_H
