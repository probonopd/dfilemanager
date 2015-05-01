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

#include <QStandardItemModel>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "globals.h"
#include "deletedialog.h"

using namespace DFM;

DeleteDialog::DeleteDialog(const QModelIndexList &idxList, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Careful!");
    QVBoxLayout *vLayout = new QVBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    QHBoxLayout *hl = new QHBoxLayout();
    QPushButton *ok = new QPushButton();
    QPushButton *cancel = new QPushButton();
    QListView *listView = new QListView();
    QStandardItemModel *model = new QStandardItemModel();
    QLabel *textLabel = new QLabel();
    QLabel *pixLabel = new QLabel();

    ok->setText("OK");
    cancel->setText("Cancel");
    listView->setModel(model);
    pixLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32));
    pixLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    hLayout->addStretch();
    hLayout->addWidget(ok);
    hLayout->addWidget(cancel);
    hLayout->addStretch();
    hl->addWidget(pixLabel);
    hl->addWidget(textLabel);
    vLayout->addLayout(hl);
    vLayout->addWidget(listView);
    vLayout->addLayout(hLayout);

    setLayout(vLayout);

    connect(ok, SIGNAL(released()) ,this, SLOT(accept()));
    connect(cancel, SIGNAL(released()), this, SLOT(reject()));

    model->clear();
    textLabel->clear();
    if (idxList.count() > 1)
        textLabel->setText("Are you sure you want to delete these " + QString::number(idxList.count()) + " items?");
    else
        textLabel->setText("Are you sure you want to delete this item?");

    foreach (const QModelIndex &index, idxList)
    {
        QStandardItem *item = new QStandardItem(index.data(FS::FileIconRole).value<QIcon>(), index.data(FS::FilePathRole).toString());
        item->setEditable(false);
        item->setSelectable(false);
        model->appendRow(item);
    }
    exec();
}
