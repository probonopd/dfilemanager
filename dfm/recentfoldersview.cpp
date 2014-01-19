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


#include "recentfoldersview.h"
#include "filesystemmodel.h"
#include "viewcontainer.h"
#include "mainwindow.h"
#include <QFileInfo>

using namespace DFM;

RecentFoldersView::RecentFoldersView(QWidget *parent) : QListView(parent), m_model(new QStandardItemModel(this))
{
    setModel(m_model);
    setIconSize(QSize(32, 32));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::NoSelection);
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));

    if (Store::config.behaviour.sideBarStyle)
    {
        //base color... slight hihglight tint
        QPalette pal = palette();
        QColor midC = Ops::colorMid( pal.color( QPalette::Base ), pal.color( QPalette::Highlight ), 10, 1 );
        pal.setColor( QPalette::Base, Ops::colorMid( Qt::black, midC, 1, 10 ) );
        setPalette( pal );
    }
}

void
RecentFoldersView::folderEntered(const QString &folder)
{
    QList<QStandardItem*> l = m_model->findItems(QFileInfo(folder).fileName());
    if ( l.count() )
        foreach ( QStandardItem *item, l )
            if ( item->data().toString() == folder )
            {
                m_model->insertRow(0, m_model->takeRow( item->row() ));
                return;
            }

    ViewContainer *vc = MainWindow::currentContainer();
    FileSystemModel *fsModel = vc->model();
    if (!fsModel)
        return;
    QIcon icon(fsModel->fileIcon(fsModel->index(folder)));
    if (icon.isNull())
        icon = QFileIconProvider().icon(QFileIconProvider::Folder);
    QStandardItem *item = new QStandardItem(icon, QFileInfo(folder).fileName());
    item->setData(folder);
    item->setToolTip(folder);
    m_model->insertRow(0, item);
}

void
RecentFoldersView::itemActivated(const QModelIndex &index)
{
    if ( index.isValid() )
        emit recentFolderClicked( m_model->itemFromIndex(index)->data().toString() );
}
