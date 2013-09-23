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


#ifndef COLUMNSVIEW_H
#define COLUMNSVIEW_H

#include <QListView>
#include <QMenu>
#include <QWheelEvent>
#include <QScrollBar>
#include <columnswidget.h>

namespace DFM
{
class ColumnsWidget;
class ColumnsView : public QListView
{
    Q_OBJECT
public:
    ColumnsView(QWidget *parent = 0, FileSystemModel *fsModel = 0, const QModelIndex &rootIndex = QModelIndex());
    ~ColumnsView(){}
    void setFilter(QString filter);
    void setModel(QAbstractItemModel *model);
    void setRootIndex(const QModelIndex &index);
    inline void setActiveFileName( const QString &fileName ) { m_activeFile = fileName; update(); }
    inline QString activeFileName() { return m_activeFile; }

public slots:
    void updateWidth();
    void dirLoaded(const QString &dir);
    void fileRenamed( const QString &path, const QString &oldName, const QString &newName );

private slots:
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    
signals:
    void newTabRequest(const QModelIndex &path);
    void focusRequest(ColumnsView *view);
    void showed(ColumnsView *view);
    void pathDeleted(ColumnsView *view);
    void dirActivated(const QModelIndex &dir);

protected:
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *event) { QListView::mousePressEvent(event); m_pressPos = event->pos(); emit focusRequest(this); }
    void focusInEvent(QFocusEvent *event) { QListView::focusInEvent(event); emit focusRequest(this); /*viewport()->update();*/ }
//    void focusOutEvent(QFocusEvent *event) { QListView::focusOutEvent(event); viewport()->update(); }
    void paintEvent(QPaintEvent *e);
    void wheelEvent(QWheelEvent *e);

private:
    ColumnsWidget *m_parent;
    FileSystemModel *m_fsModel;
    QFileSystemWatcher *m_fsWatcher;
    QPoint m_pressPos;
    QString m_activeFile, m_rootPath;
    int m_width;
    bool m_isSorted;
};

}

#endif // COLUMNSVIEW_H
