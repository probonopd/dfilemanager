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

#include <QDirIterator>
#include <QList>

#include "fsworkers.h"
#include "fsmodel.h"
#include "fsnode.h"
#include "helpers.h"
#include "devices.h"

using namespace DFM;
using namespace FS;
using namespace Worker;

Gatherer::Gatherer(QObject *parent)
    :DThread(parent)
    , m_node(0)
    , m_model(static_cast<FS::Model *>(parent))
    , m_isCancelled(false)
{
    connect(this, SIGNAL(started()), m_model, SIGNAL(startedWorking()));
    connect(this, SIGNAL(finished()), m_model, SIGNAL(finishedWorking()));
}

void
Gatherer::setCancelled(bool cancel)
{
    m_pausingMutex.lock();
    m_isCancelled=cancel;
    m_pausingMutex.unlock();
}

void
Gatherer::run()
{
    switch (m_task)
    {
    case Populate: if (m_node) m_node->rePopulate(); break;
    case Generate:
    {
        if (QFileInfo(m_path).isDir())
            m_result = m_node->localNode(m_path, false);
        if (m_result)
            emit nodeGenerated(m_path, m_result);
        break;
    }
    case Search: searchResultsForNode(m_searchName, m_searchPath, m_node); break;
    case GetApps: getApplications(m_appsPath, m_node); break;
    default: break;
    }
    m_taskMutex.lock();
    m_node = 0;
    m_task = NoTask;
    m_taskMutex.unlock();
}

void
Gatherer::search(const QString &name, const QString &path, Node *node)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = Search;
    m_node = node;
    m_searchName = name;
    m_searchPath = path;
    start();
}

void
Gatherer::populateApplications(const QString &appsPath, Node *node)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = GetApps;
    m_node = node;
    m_appsPath = appsPath;
    start();
}


void
Gatherer::populateNode(Node *node)
{
    if (m_task == Populate && node == m_node) //already populating the node....
        return;

    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = Populate;
    m_node = node;
    start();
}

void
Gatherer::generateNode(const QString &path, Node *parent)
{
    setCancelled(true);
    wait();
    setCancelled(false);

    QMutexLocker locker(&m_taskMutex);
    m_task = Generate;
    m_path = path;
    m_node = parent;
    m_result = 0;
    start();
}

void
Gatherer::getApplications(const QString &appsPath, Node *node)
{
    QDirIterator it(appsPath, allEntries, QDirIterator::Subdirectories);
    while (it.hasNext()&&!m_model->m_dataGatherer->isCancelled())
    {
        const QString &file(it.next());
        if (QFileInfo(file).isDir())
            continue;
        const QUrl &url = QUrl(QString("%1%2").arg(m_model->m_url.toString(), file));
        new AppNode(m_model, node, url, file);
    }
    emit m_model->urlLoaded(m_model->m_url);
}

void
Gatherer::searchResultsForNode(const QString &name, const QString &filePath, Node *node)
{
    QDirIterator it(filePath, allEntries, QDirIterator::Subdirectories);
    while (it.hasNext()&&!m_model->m_dataGatherer->isCancelled())
    {
        const QFileInfo &file(it.next());
        if (file.fileName().contains(name, Qt::CaseInsensitive))
            new Node(m_model, QUrl::fromLocalFile(file.filePath()), node, file.filePath());
    }
    emit m_model->urlLoaded(m_model->m_url);
}
