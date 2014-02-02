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

/* A collection of classes that's based on QObject thats used
 * all around in the code.
 */

#ifndef OBJECTS_H
#define OBJECTS_H

#include <QThread>
#include <QStyledItemDelegate>
#include <QWaitCondition>
#include <QMutex>

namespace DFM
{

class Thread : public QThread
{
    Q_OBJECT
public:
    explicit Thread(QObject *parent = 0);
    void setPause(bool p);
    bool isPaused();
    void pause();

public slots:
    virtual void discontinue();

protected:
    QWaitCondition m_pauseCond;
    QMutex m_mutex;
    bool m_pause;
    bool m_quit;
};

class FileItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    FileItemDelegate(QObject *parent = 0):QStyledItemDelegate(parent){}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    bool eventFilter(QObject *object, QEvent *event);
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

}

#endif // OBJECTS_H
