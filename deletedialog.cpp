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


#include "deletedialog.h"
#include "mainwindow.h"

using namespace DFM;

DeleteDialog::DeleteDialog(const QModelIndexList &idxList, QWidget *parent) : QDialog(parent)
{
    m_fsModel = MainWindow::currentWindow()->currentContainer()->model();
    setWindowTitle("Careful!");
    m_vLayout = new QVBoxLayout;
    m_hLayout = new QHBoxLayout;
    m_hl = new QHBoxLayout;
    m_ok = new QPushButton;
    m_cancel = new QPushButton;
    m_listView = new QListView;
    m_model = new QStandardItemModel;
    m_textLabel = new QLabel;
    m_pixLabel = new QLabel;

    m_ok->setText("OK");
    m_cancel->setText("Cancel");
    m_listView->setModel(m_model);
    m_pixLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32));
    m_pixLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    m_hLayout->addStretch();
    m_hLayout->addWidget(m_ok);
    m_hLayout->addWidget(m_cancel);
    m_hLayout->addStretch();
    m_hl->addWidget(m_pixLabel);
    m_hl->addWidget(m_textLabel);
    m_vLayout->addLayout(m_hl);
    m_vLayout->addWidget(m_listView);
    m_vLayout->addLayout(m_hLayout);

    setLayout(m_vLayout);

    connect( m_ok,SIGNAL(released()),this,
             SLOT(accept()));
    connect( m_cancel, SIGNAL(released()),this,
             SLOT(reject()));

    m_model->clear();
    m_textLabel->clear();
    if(idxList.count() > 1)
        m_textLabel->setText("Are you sure you want to delete these " + QString::number(idxList.count()) + " items?");
    else
        m_textLabel->setText("Are you sure you want to delete this item?");

    foreach(QModelIndex index, idxList)
    {
        QStandardItem *item = new QStandardItem(m_fsModel->fileIcon(index), m_fsModel->filePath(index));
        item->setEditable(false);
        item->setSelectable(false);
        m_model->appendRow(item);
    }
    exec();
}
