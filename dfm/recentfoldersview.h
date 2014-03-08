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


#ifndef RECENTFOLDERSVIEW_H
#define RECENTFOLDERSVIEW_H

#include <QListView>
#include <QStandardItemModel>

namespace DFM
{

class RecentFoldersModel : public QStandardItemModel
{
public:
    inline explicit RecentFoldersModel(QObject *parent = 0):QStandardItemModel(parent){}
    QVariant data(const QModelIndex &index, int role) const;
};

class RecentFoldersView : public QListView
{
    Q_OBJECT
public:
    explicit RecentFoldersView(QWidget *parent = 0);
    
signals:
    void recentFolderClicked(const QUrl &url);
    
public slots:
    void folderEntered(const QUrl &url);
private slots:
    void itemActivated(const QModelIndex &index);
    void paletteOps();
private:
    RecentFoldersModel *m_model;
    friend class RecentFoldersModel;
};

}

#endif // RECENTFOLDERSVIEW_H
