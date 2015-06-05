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


#ifndef FSWORKERS_H
#define FSWORKERS_H

#include <QFileInfo>

#include "objects.h"
#include "dataloader.h"
#include "devices.h"

namespace DFM
{
namespace FS
{
class Node;
class Model;
namespace Worker
{
enum Task { Populate = 0, Generate, Search, GetApps, NoTask };

class Job
{
public:
    Job(const Task &t, Node *node, const QString &path)
        : m_task(t)
        , m_node(node)
        , m_path(path)
    {
    }
    Node *m_node;
    QString m_path;
    Task m_task;
};

typedef QList<Job> Jobs;

class Gatherer : public DThread
{
    Q_OBJECT
public:
    explicit Gatherer(QObject *parent = 0);
    void populateNode(Node *node);
    void generateNode(const QString &path, Node *parent);
    void search(const QString &name, const QString &filePath, Node *node);
    void populateApplications(const QString &appsPath, Node *node);
    void setCancelled(bool cancel);
    bool isCancelled() { QMutexLocker locker(&m_pausingMutex); return m_isCancelled; }

protected:
    void run();
    void searchResultsForNode(const QString &name, const QString &filePath, Node *node);
    void getApplications(const QString &appsPath, Node *node);

signals:
    void nodeGenerated(const QString &path, Node *node);

private:
    QString m_path, m_searchPath, m_searchName, m_appsPath;
    QMutex m_taskMutex;
    Node *m_node, *m_result;
    FS::Model *m_model;
    Task m_task;
    bool m_isCancelled;
    friend class Node;
    friend class FS::Model;
};

} //namespace worker

} //namespace fs

}

#endif // FSWORKERS_H
