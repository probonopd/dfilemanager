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


#ifndef DATALOADER_H
#define DATALOADER_H

#include <QObject>
#include <QHash>
#include <QThread>
#include <QModelIndex>
#include <QDebug>
#include <QImageReader>
#include <QAbstractItemView>
#include <QFile>
#include <QDir>
#include <QEvent>
#include <QFileSystemWatcher>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>
#include "filesystemmodel.h"
#include "interfaces.h"
#include "objects.h"
#include "helpers.h"

namespace DFM
{

struct Data
{
    QImage thumb;
    QString count; //entries for dirs...
    QString mimeType, iconName, lastModified, fileType;
};

class DataLoader : public Thread
{
    Q_OBJECT
public:
    static DataLoader *instance();

    static inline void clearQueue() { instance()->_clearQueue(); }
    static inline void removeData(const QString &file) { instance()->_removeData(file); }
    static inline struct Data *data(const QString &file, const bool checkOnly = false) { return instance()->_data(file, checkOnly); }
    bool hasData(const QString &file) const;
    struct Data *fetchData(const QString &file);

public slots:
    void discontinue() { m_mtx.lock(); m_queue.clear(); m_mtx.unlock(); Thread::discontinue(); }
    void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
    void _removeData(const QString &file);
    
signals:
    void newData(const QString &file);
    void noLongerExists(const QString &file);

protected:
    explicit DataLoader(QObject *parent = 0);
    void run();
    void getData(const QString &path);
    void _clearQueue();
    bool hasQueue() const;
    bool enqueue(const QString &file);
    QString dequeue();
    Data *_data(const QString &file, const bool checkOnly);

private:
    mutable QMutex m_mtx;
    QHash<QString, Data *> m_data;
    QQueue<QString> m_queue;
    int m_extent;
    MimeProvider m_mimeProvider;
    static DataLoader *m_instance;
};

}

#endif // DATALOADER_H
