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

#include <QFileInfo>
#include <QModelIndex>
#include <QTimer>

#include "recentfoldersview.h"
#include "fsmodel.h"
#include "config.h"
#include "dataloader.h"
#include "application.h"
#include "operations.h"

using namespace DFM;

QVariant
RecentFoldersModel::data(const QModelIndex &index, int role) const
{
    QStandardItem *item = itemFromIndex(index);
    if (!item)
        return QVariant();
    if (role == Qt::DecorationRole)
    {
        const QString &file = item->data().toString();
        Data *d = DDataLoader::data(file);
        if (d && QIcon::hasThemeIcon(d->iconName))
            return QIcon::fromTheme(d->iconName);
#if defined(ISWINDOWS)
        return FS::FileIconProvider::fileIcon(QFileInfo(file));
#elif defined(ISUNIX)
        return FS::FileIconProvider::typeIcon(FS::FileIconProvider::Folder);
#endif
    }
    return QStandardItemModel::data(index, role);
}

RecentFoldersView::RecentFoldersView(QWidget *parent) : QListView(parent), m_model(new RecentFoldersModel(this))
{
    setModel(m_model);
    setIconSize(QSize(32, 32));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::NoSelection);
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));

    QTimer::singleShot(0, this, SLOT(paletteOps()));
}

void
RecentFoldersView::paletteOps()
{
    QPalette::ColorRole bg = viewport()->backgroundRole(), fg = viewport()->foregroundRole();
    if (Store::config.behaviour.sideBarStyle == -1)
    {
        viewport()->setAutoFillBackground(false);
        setFrameStyle(0);
    }
    if (!viewport()->autoFillBackground())
    {
        bg = backgroundRole();
        fg = foregroundRole();
    }
    setBackgroundRole(bg);
    setForegroundRole(fg);
    viewport()->setBackgroundRole(bg);
    viewport()->setForegroundRole(fg);
    QPalette pal = palette();
    const QColor fgc = pal.color(fg), bgc = pal.color(bg);
    if (Store::config.behaviour.sideBarStyle == 1)
    {
        viewport()->setAutoFillBackground(true);
        //base color... slight hihglight tint
        QColor midC = Ops::colorMid(pal.color(QPalette::Base), qApp->palette().color(QPalette::Highlight), 10, 1);
        pal.setColor(bg, Ops::colorMid(Qt::black, midC, 255-pal.color(QPalette::Window).value(), 255));
        pal.setColor(fg, qApp->palette().color(fg));
    }
    else if (Store::config.behaviour.sideBarStyle == 2)
    {
        pal.setColor(bg, Ops::colorMid(fgc, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, bgc);
    }
    else if (Store::config.behaviour.sideBarStyle == 3)
    {
        const QColor &wtext = pal.color(QPalette::WindowText), w = pal.color(QPalette::Window);
        pal.setColor(bg, Ops::colorMid(wtext, qApp->palette().color(QPalette::Highlight), 10, 1));
        pal.setColor(fg, w);
    }
    setPalette(pal);
}

void
RecentFoldersView::folderEntered(const QUrl &url)
{
    const QString &folder = url.toLocalFile();
    if (!QFileInfo(folder).exists())
        return;
    const QString &folderName = QFileInfo(folder).fileName().isEmpty()?folder:QFileInfo(folder).fileName();
    QList<QStandardItem *> l = m_model->findItems(folderName);
    if (l.count())
        foreach (QStandardItem *item, l)
            if (item->data().toString() == folder)
            {
                m_model->insertRow(0, m_model->takeRow(item->row()));
                return;
            }
    QStandardItem *item = new QStandardItem(folderName);
    item->setData(folder);
    item->setToolTip(folder);
    m_model->insertRow(0, item);
}

void
RecentFoldersView::itemActivated(const QModelIndex &index)
{
    if (index.isValid())
        emit recentFolderClicked(QUrl::fromLocalFile(m_model->itemFromIndex(index)->data().toString()));
}
