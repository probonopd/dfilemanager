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


#ifndef DELETEDIALOG_H
#define DELETEDIALOG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <filesystemmodel.h>

namespace DFM
{

class FileSystemModel;
class DeleteDialog : public QDialog
{
    Q_OBJECT
public:
    DeleteDialog(const QModelIndexList &idxList, QWidget *parent = 0);
    
private:
    QStandardItemModel *m_model;
    QListView *m_listView;
    QPushButton *m_ok, *m_cancel;
    QVBoxLayout *m_vLayout;
    QHBoxLayout *m_hLayout, *m_hl;
    QLabel *m_textLabel, *m_pixLabel;
    FileSystemModel *m_fsModel;
};

}

#endif // DELETEDIALOG_H
