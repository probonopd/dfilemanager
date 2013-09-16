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
    void setModel( FileSystemModel *model );
    inline void setSelectionModel( QItemSelectionModel *model ) { m_slctModel = model; }
    QModelIndex currentIndex();
    ColumnsView *currentView();
    void clear();
    void setFilter( const QString &filter );
    void scrollTo(const QModelIndex &index);
    
signals:
    void currentViewChagned( ColumnsView *view );
    
public slots:
    void rootPathChanged( const QString &rootPath );
    void edit(const QModelIndex &index);
    void setRootIndex( const QModelIndex &index );
    void setCurrentView( ColumnsView *view );

protected:
    void connectView(ColumnsView *view);
    void disconnectView(ColumnsView *view);
    void resizeEvent(QResizeEvent *e);
    void showEvent(QShowEvent *e);
    ColumnsView *newView(const QModelIndex &index);
    QModelIndexList fromRoot();
    QModelIndexList fromLast();

private:
    FileSystemModel *m_fsModel;
    QItemSelectionModel *m_slctModel;
    QWidget *m_viewport;
    QHBoxLayout *m_viewLay;
    QMap<QModelIndex, ColumnsView *> m_views;
    ViewContainer *m_container;
    ColumnsView *m_currentView;
    QModelIndex m_rootIndex;
    QString m_rootPath;
};

}

#endif // COLUMNSWIDGET_H
