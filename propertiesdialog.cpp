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


#include "propertiesdialog.h"
#include <QGroupBox>

PropertiesDialog::PropertiesDialog(QWidget *parent, QString filePath)
    : QDialog(parent)
    , ownerRead(new QCheckBox(this))
    , ownerWrite(new QCheckBox(this))
    , ownerExecute(new QCheckBox(this))
    , groupRead(new QCheckBox(this))
    , groupWrite(new QCheckBox(this))
    , groupExecute(new QCheckBox(this))
    , othersRead(new QCheckBox(this))
    , othersWrite(new QCheckBox(this))
    , othersExecute(new QCheckBox(this))
{
    QFileInfo fileInfo(filePath);
    QGroupBox *gboRead = new QGroupBox("Owner");

    if (filePath != QString())
        enableButtons(fileInfo);

    setWindowTitle(fileInfo.fileName());

    currentFile = filePath;

    ownerRead->setText(tr("read"));
    ownerWrite->setText(tr("write"));
    ownerExecute->setText(tr("execute"));

    QVBoxLayout *voLayout = new QVBoxLayout;
    voLayout->addWidget(ownerRead);
    voLayout->addWidget(ownerWrite);
    voLayout->addWidget(ownerExecute);
    gboRead->setLayout(voLayout);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(gboRead);

    QGroupBox *gbg = new QGroupBox("Group");

    groupRead->setText(tr("read"));
    groupWrite->setText(tr("write"));
    groupExecute->setText(tr("execute"));

    QVBoxLayout *vgLayout = new QVBoxLayout;
    vgLayout->addWidget(groupRead);
    vgLayout->addWidget(groupWrite);
    vgLayout->addWidget(groupExecute);
    gbg->setLayout(vgLayout);

    hLayout->addWidget(gbg);

    QGroupBox *gbOther = new QGroupBox("Other");

    othersRead->setText(tr("read"));
    othersWrite->setText(tr("write"));
    othersExecute->setText(tr("execute"));

    QVBoxLayout *vOthLayout = new QVBoxLayout;
    vOthLayout->addWidget(othersRead);
    vOthLayout->addWidget(othersWrite);
    vOthLayout->addWidget(othersExecute);
    gbOther->setLayout(vOthLayout);

    hLayout->addWidget(gbOther);

    QPushButton *ok = new QPushButton(tr("OK"),this);
    QPushButton *cancel = new QPushButton(tr("Cancel"),this);

    connect(ok,SIGNAL(released()),this,SLOT(accept()));
    connect(cancel,SIGNAL(released()),this,SLOT(reject()));

    QHBoxLayout *btns = new QHBoxLayout();
    btns->addWidget(ok);
    btns->addWidget(cancel);

    QVBoxLayout *vbox = new QVBoxLayout();

    vbox->addLayout(hLayout);
    vbox->addLayout(btns);

    setLayout(vbox);
}

void
PropertiesDialog::enableButtons(QFileInfo fileInfo)
{
    currentFile = fileInfo.filePath();
    setWindowTitle(fileInfo.fileName());

    ownerRead->setEnabled(fileInfo.isWritable());
    ownerWrite->setEnabled(fileInfo.isWritable());
    ownerExecute->setEnabled(fileInfo.isWritable());
    groupRead->setEnabled(fileInfo.isWritable());
    groupWrite->setEnabled(fileInfo.isWritable());
    groupExecute->setEnabled(fileInfo.isWritable());
    othersRead->setEnabled(fileInfo.isWritable());
    othersWrite->setEnabled(fileInfo.isWritable());
    othersExecute->setEnabled(fileInfo.isWritable());

    if(fileInfo.isDir())
    {
        ownerExecute->setEnabled(false);
        groupExecute->setEnabled(false);
        othersExecute->setEnabled(false);
    }

    ownerRead->setChecked(fileInfo.permission(QFile::ReadOwner));
    ownerWrite->setChecked(fileInfo.permission(QFile::WriteOwner));
    ownerExecute->setChecked(fileInfo.permission(QFile::ExeOwner));
    groupRead->setChecked(fileInfo.permission(QFile::ReadGroup));
    groupWrite->setChecked(fileInfo.permission(QFile::WriteGroup));
    groupExecute->setChecked(fileInfo.permission(QFile::ExeGroup));
    othersRead->setChecked(fileInfo.permission(QFile::ReadOther));
    othersWrite->setChecked(fileInfo.permission(QFile::WriteOther));
    othersExecute->setChecked(fileInfo.permission(QFile::ExeOther));
}

void
PropertiesDialog::accept()
{
    QFile file(currentFile);
    QFileInfo fileInfo(currentFile);
    if(fileInfo.isWritable())
    {
        QFlags<QFile::Permission> permissions = file.permissions();
        if(ownerRead->isChecked()) permissions |= QFile::ReadOwner; else permissions &= ~QFile::ReadOwner;
        if(ownerWrite->isChecked()) permissions |= QFile::WriteOwner; else permissions &= ~QFile::WriteOwner;
        if(ownerExecute->isChecked()) permissions |= QFile::ExeOwner; else permissions &= ~QFile::ExeOwner;
        if(groupRead->isChecked()) permissions |= QFile::ReadGroup; else permissions &= ~QFile::ReadGroup;
        if(groupWrite->isChecked()) permissions |= QFile::WriteGroup; else permissions &= ~QFile::WriteGroup;
        if(groupExecute->isChecked()) permissions |= QFile::ExeGroup; else permissions &= ~QFile::ExeGroup;
        if(othersRead->isChecked()) permissions |= QFile::ReadOther; else permissions &= ~QFile::ReadOther;
        if(othersWrite->isChecked()) permissions |= QFile::WriteOther; else permissions &= ~QFile::WriteOther;
        if(othersExecute->isChecked()) permissions |= QFile::ExeOther; else permissions &= ~QFile::ExeOther;
        qDebug() << permissions;
        qDebug() << file.permissions();
        file.setPermissions(permissions);
    }
    QDialog::accept();
}
