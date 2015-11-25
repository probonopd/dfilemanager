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


#include "icondialog.h"
#include "globals.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QDirIterator>
#include <QImageReader>
#include <QDebug>

using namespace DFM;

IconDialog::IconDialog(QWidget *parent) : QDialog(parent)
{
    m_ok = new QPushButton(this);
    m_model = new QStandardItemModel(this);
    m_listView = new QListView(this);
    m_listView->setModel(m_model);
    m_listView->setIconSize(QSize(16,16));
    setWindowTitle(tr("Select icon"));


    QPushButton *cancel = new QPushButton(this);
    m_ok->setText("OK");
    cancel->setText("Cancel");

    QVBoxLayout *verticalLayout = new QVBoxLayout();
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    verticalLayout->addWidget(m_listView);
    horizontalLayout->addStretch();
    horizontalLayout->addWidget(m_ok);
    horizontalLayout->addWidget(cancel);
    horizontalLayout->addStretch();
    verticalLayout->addLayout(horizontalLayout);
    setLayout(verticalLayout);

    connect(m_listView, SIGNAL(activated(QModelIndex)), this, SLOT(enableOkButton()));
    connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SLOT(enableOkButton()));
    connect(m_ok, SIGNAL(released()), this, SLOT(accept()));
    connect(cancel, SIGNAL(released()), this, SLOT(reject()));
    enableOkButton();
}

QString IconDialog::getIcon()
{
    QList<QFileInfo> files;
    for (int i = 0; i < QIcon::themeSearchPaths().count(); i++)
    {
        QString theme(QIcon::themeSearchPaths().at(i));
#if QT_VERSION >= 0x050000
        theme.append("/");
#endif
        theme.append(QIcon::themeName());
        files << theme;
    }

    if (files.isEmpty())
    {
        QMessageBox::information(this, "Could not find any valid icontheme path!", "Cannot continue, exiting");
        return QString();
    }

    QStringList list;
    QMap<QString, QIcon> icons;
    foreach (const QFileInfo &file, files)
    {
        QDirIterator it(file.absoluteFilePath(), allEntries, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            const QFileInfo fi(it.next());
            if (!fi.filePath().toLower().contains("places")
                    || !fi.baseName().contains("-")
                    || list.contains(fi.baseName()))
                continue;
            QIcon icon(QPixmap::fromImage(QImageReader(fi.filePath()).read()));
            list << fi.baseName();
            icons.insert(fi.baseName(), icon);
        }
    }
    qStableSort(list);
    for (int i = 0; i < list.count(); ++i)
        m_model->appendRow(new QStandardItem(icons.value(list.at(i)), list.at(i)));

    exec();
    if (result() == Accepted)
        return m_listView->currentIndex().data().toString();
    return QString();
}
