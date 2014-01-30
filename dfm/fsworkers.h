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

#include "objects.h"
#include "filesystemmodel.h"

namespace DFM
{

class FileSystemModel;
namespace Worker { class Gatherer; }

class FileSystemNode : public QFileInfo
{
public:
    typedef QList<FileSystemNode *> Nodes;
    enum Children { Visible = 0, Hidden = 1, Filtered = 2, Files = 3, HiddenFiles = 4 };
    enum Type { FSNode = 0, SearchResult = 1 };
    ~FileSystemNode();
    void insertChild(FileSystemNode *node);
    inline QString filePath() const { return m_filePath; }

    inline QString scheme() const { return m_scheme; }
    inline void setScheme(const QString &scheme) { m_scheme = scheme; }

    inline bool isSearchResult() { return m_type == SearchResult; }

    bool hasChild(const QString &name, const bool nameIsPath = false, const bool onlyVisible = false);
    int childCount() const;
    int row();

    inline Type type() const { return m_type; }

    FileSystemNode *child(const int c);
    FileSystemNode *child(const QString &path);
    inline FileSystemNode *parent() const { return m_parent; }

    void rePopulate();
    inline bool isPopulated() const { return m_isPopulated; }

    QString name();
    bool rename(const QString &newName);

    QVariant data(const int column);

    QString permissionsString();
    QString category();

    void sort();
    int sortColumn();
    Qt::SortOrder sortOrder();

    void setHiddenVisible(bool visible);
    bool showHidden();

    void setFilter(const QString &filter);
    inline QString filter() const { return m_filter; }

    FileSystemNode *node(const QString &path, bool checkOnly = true);
    FileSystemModel *model();
    Worker::Gatherer *gatherer();

protected:
    int rowFor(FileSystemNode *child) const;
    void search(const QString &fileName);
    void endSearch();
    FileSystemNode *nodeFromPath(const QString &path, bool checkOnly = true);
    FileSystemNode(FileSystemModel *model = 0, const QString &path = QString(), FileSystemNode *parent = 0, const Type &t = FSNode);

private:
    bool m_isPopulated;
    FileSystemNode *m_parent;
    Nodes m_children[HiddenFiles+1];
    QString m_filePath, m_filter, m_name, m_scheme;
    FileSystemModel *m_model;
    Type m_type;
    mutable QMutex m_mutex;
    friend class FileSystemModel;
    friend class Worker::Gatherer;
};

namespace Worker
{
enum Task { Populate = 0, Generate = 1, Search = 2, NoTask = 3 };

class Job
{
public:
    Job(const Task &t, FileSystemNode *node, const QString &path)
        : m_task(t)
        , m_node(node)
        , m_path(path)
    {
    }
    FileSystemNode *m_node;
    QString m_path;
    Task m_task;
};

typedef QList<Job> Jobs;

class Gatherer : public Thread
{
    Q_OBJECT
public:
    explicit Gatherer(QObject *parent = 0);
    void populateNode(FileSystemNode *node);
    void generateNode(const QString &path);
    void search(const QString &name, FileSystemNode *node);
    void setCancelled(bool cancel) { m_mutex.lock(); m_isCancelled=cancel; m_mutex.unlock(); }
    bool isCancelled() { QMutexLocker locker(&m_mutex); return m_isCancelled; }

protected:
    void run();

signals:
    void nodeGenerated(const QString &path, FileSystemNode *node);

private:
    QMutex m_taskMutex;
    FileSystemNode *m_node, *m_result;
    FileSystemModel *m_model;
    Task m_task;
    QString m_path, m_searchPath, m_searchName;
    bool m_isCancelled;
    friend class FileSystemNode;
    friend class FileSystemModel;
};

}

}

#endif // FSWORKERS_H
