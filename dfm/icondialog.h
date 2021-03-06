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


#ifndef ICONDIALOG_H
#define ICONDIALOG_H

#include <QDialog>
#include <QListView>
#include <QStandardItemModel>
#include <QLayout>
#include <QPushButton>

namespace DFM
{

class IconDialog : public QDialog
{
    Q_OBJECT
public:
    explicit IconDialog(QWidget *parent = 0);
    static inline QString iconName() { return IconDialog().getIcon(); }
    QString getIcon();

private slots:
    inline void enableOkButton() { m_ok->setEnabled(m_listView->selectionModel()->hasSelection()); }

private:
    QPushButton *m_ok;
    QListView *m_listView;
    QStandardItemModel *m_model;
    QHBoxLayout *m_horizontalLayout;
    QVBoxLayout *m_verticalLayout;
};

}

#endif // ICONDIALOG_H
