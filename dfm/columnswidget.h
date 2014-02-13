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

#ifndef COLUMNSWIDGET_H
#define COLUMNSWIDGET_H

#include <QScrollArea>
#include <QHBoxLayout>
#include "filesystemmodel.h"
#include "columnsview.h"
#include "viewcontainer.h"

namespace DFM
{
class ViewContainer;
class ColumnsView;
class ColumnsWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit ColumnsWidget(QWidget *parent = 0);
    ~ColumnsWidget(){}
    void setModel(QAbstractItemModel *model);
    inline void setSelectionModel(QItemSelectionModel *model) { m_slctModel = model; }
    QModelIndex currentIndex();
    ColumnsView *currentView();
    ViewContainer *container() { return m_container; }
    void clear();
    void scrollTo(const QModelIndex &index);
    
signals:
    void currentViewChagned(ColumnsView *view);
    
public slots:
    void edit(const QModelIndex &index);
    void setRootIndex(const QModelIndex &index);
    void setCurrentView(ColumnsView *view);
    void reconnectViews();
    void showCurrent();

private slots:
    void expand(const QModelIndex &index);

protected:
    void connectView(ColumnsView *view);
    void showEvent(QShowEvent *e);
    void wheelEvent(QWheelEvent *e);
    int at(const QUrl &url) { return m_rootList.indexOf(url); }
    inline bool isValid(const QUrl &url) { return !m_columns.isEmpty()&&at(url)<m_columns.count(); }
    inline bool isValid(const int i) { return i>-1&&!m_columns.isEmpty()&&i<m_columns.count(); }
    inline ColumnsView *column(const int i) { return isValid(i) ? m_columns.at(i): 0; }
    inline ColumnsView *column(const QUrl &url) { return isValid(url) ? column(at(url)) : 0; }
    inline int count() { return m_columns.count(); }

private:
    FS::Model *m_model;
    QItemSelectionModel *m_slctModel;
    QFrame *m_viewport;
    QHBoxLayout *m_viewLay;
    QList<ColumnsView *> m_columns;
    ViewContainer *m_container;
    ColumnsView *m_currentView;
    QModelIndex m_rootIndex;
    QList<QUrl> m_rootList;
};

}

#endif // COLUMNSWIDGET_H
