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
#include <QSortFilterProxyModel>

namespace DFM
{
class ColumnsWidget;
class ColumnsView : public QListView
{
    Q_OBJECT
public:
    ColumnsView(QWidget *parent = 0, QAbstractItemModel *model = 0, const QUrl &url = QUrl());
    ~ColumnsView(){}
    void setModel(QAbstractItemModel *model);
//    void setRootIndex(const QModelIndex &index);
    inline void setActiveFileName(const QString &fileName) { QFileInfo f(fileName); m_activeFile = f.exists()?f.fileName():fileName; update(); }
    inline QString activeFileName() { return m_activeFile; }
    inline QUrl rootUrl() { return m_url; }
    void setUrl(const QUrl &url);
    QRect expanderRect(const QModelIndex &index);

public slots:
//    void updateWidth();
//    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);

private slots:
//    void rowsRemoved(const QModelIndex &parent, int start, int end);
    
signals:
    void newTabRequest(const QModelIndex &index);
    void focusRequest(ColumnsView *view);
    void showed(ColumnsView *view);
    void pathDeleted(ColumnsView *view);
    void dirActivated(const QModelIndex &dir);
    void expandRequest(const QModelIndex &index);
    void opened(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *event) { QListView::mousePressEvent(event); m_pressPos = event->pos(); emit focusRequest(this); }
    void mouseDoubleClickEvent(QMouseEvent *event);
//    void focusInEvent(QFocusEvent *event) { QListView::focusInEvent(event); emit focusRequest(this); /*viewport()->update();*/ }
//    void focusOutEvent(QFocusEvent *event) { QListView::focusOutEvent(event); viewport()->update(); }
    void wheelEvent(QWheelEvent *e);
    bool eventFilter(QObject *o, QEvent *e);
//    bool sanityCheckForDir();

private:
    ColumnsWidget *m_parent;
    FS::Model *m_model;
    QSortFilterProxyModel *m_sortModel;
    QFileSystemWatcher *m_fsWatcher;
    QPoint m_pressPos;
    QString m_activeFile;
    QUrl m_url;
    int m_width;
};

}

#endif // COLUMNSVIEW_H
