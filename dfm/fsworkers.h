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
#include "filesystemmodel.h"

namespace DFM
{

namespace FS
{

class Model;
namespace Worker { class Gatherer; }

class Node;
typedef QList<Node *> Nodes;

class Node : public QFileInfo
{
    friend class Model;
    friend class Worker::Gatherer;
public:
    enum Children { Visible = 0, Hidden = 1, Filtered = 2, ChildrenTypeCount = 3 };
    Node(FS::Model *model = 0, const QUrl &url = QUrl(), Node *parent = 0, const QUrl &localUrl = QUrl());
    ~Node();

    inline Model *model() { return m_model; }
    Worker::Gatherer *gatherer();

    inline QString name() const { return m_name; }
    bool rename(const QString &newName);
    inline QString filePath() const { return m_filePath; }
    inline bool isSearchResult() { return false; }

    int row();
    int childCount() const;
    void addChild(Node *child);
    void addChild(const QUrl &url);
    Node *child(const QUrl &url, const bool onlyVisible = false);
    Node *child(const int c);
    Node *child(const QString &name, const bool nameIsPath = true);
    inline bool hasChildren() const { return !m_children[Visible].isEmpty(); }

    void populateScheme() {}
    void removeDeleted();
    void rePopulate();
    inline bool isPopulated() const { return m_isPopulated; }

    QVariant data(int column);
    QString permissionsString();
    QString category();
    inline QString scheme() { return url().scheme(); }
    QIcon icon();

    void sort();
    int sortColumn();
    Qt::SortOrder sortOrder();

    void setHiddenVisible(bool visible);
    bool showHidden();

    void setFilter(const QString &filter);
    inline QString filter() const { return m_filter; }

    void clearVisible();

    Node *nodeFromUrl(const QUrl &url);
    Node *localNode(const QString &path, bool checkOnly = true);
    Node *nodeFromLocalPath(const QString &path, bool checkOnly = true);
    Node *nodeFromUrlPath(const QString &path);

    inline QUrl url() { return m_url; }
    inline QUrl localUrl() { return m_localUrl; }
    inline void setUrl(const QUrl &url) { m_url = url; }
    inline void setLocalUrl(const QUrl &url) { m_localUrl = url; }

    Node *parent() { return m_parent; }

private:
    bool m_isPopulated;
    Nodes m_children[ChildrenTypeCount];
    Node *m_parent;
    QString m_filePath, m_filter, m_name;
    QUrl m_url, m_localUrl;
    mutable QMutex m_mutex;
    Model *m_model;
};

namespace Worker
{
enum Task { Populate = 0, Generate = 1, Search = 2, NoTask = 3 };

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

class Gatherer : public Thread
{
    Q_OBJECT
public:
    explicit Gatherer(QObject *parent = 0);
    void populateNode(Node *node);
    void generateNode(const QString &path, Node *parent);
    void search(const QString &name, const QString &filePath, Node *node);
    void setCancelled(bool cancel);
    bool isCancelled() { QMutexLocker locker(&m_mutex); return m_isCancelled; }

protected:
    void run();
    void searchResultsForNode(const QString &name, const QString &filePath, Node *node);
    void loadLocalDir(Node *node);
    Node *generateLocalNode(const QUrl &localUrl, Node *from);
    Node *nodeFromLocalPath(const QString &path, Node *parent, const bool checkOnly = true);

signals:
    void nodeGenerated(const QString &path, Node *node);

private:
    QString m_path, m_searchPath, m_searchName;
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
