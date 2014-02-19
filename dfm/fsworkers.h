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
    Node(FS::Model *model = 0, const QUrl &url = QUrl(), Node *parent = 0, const QString &filePath = QString());
    ~Node();

    inline Model *model() { return m_model; }
    Worker::Gatherer *gatherer();

    virtual QString name() const { return m_name; }
    bool rename(const QString &newName);
    inline QString filePath() const { return m_filePath; }

    int row();
    int rowOf(Node *child) const;
    int childCount(Children children = Visible) const;
    void addChild(Node *node);
    Node *child(const int c, Children fromChildren = Visible);
    bool hasChild(const QString &name, const bool nameIsPath = true);
    inline bool hasChildren() const { return !m_children[Visible].isEmpty(); }
    void insertChild(Node *n, const int i);

    void removeDeleted();
    void rePopulate();
    bool isPopulated() const;

    virtual QVariant data(const int column);
    QString permissionsString();
    virtual QString category();
    inline QString scheme() { return url().scheme(); }
    QString iconName();
    QString mimeType();
    bool isExec();
    virtual QIcon icon();
    virtual void exec();
    void sort();
    int sortColumn();
    Qt::SortOrder sortOrder();

    void setHiddenVisible(bool visible);
    bool showHidden();

    void setFilter(const QString &filter);
    inline QString filter() const { return m_filter; }

    void clearVisible();
    void removeChild(Node *node);

    Node *localNode(const QString &path, bool checkOnly = true);
    Node *nodeFromLocalPath(const QString &path, bool checkOnly = true);
    inline QUrl url() const { return m_url; }
    inline void setUrl(const QUrl &url) { m_url = url; }

    Node *parent() { return m_parent; }

private:
    bool m_isPopulated;
    int m_isExe;
    Nodes m_children[ChildrenTypeCount];
    Node *m_parent;
    QString m_filePath, m_filter, m_name, m_mimeType, m_iconName;
    QUrl m_url;
    mutable QMutex m_mutex;
    Model *m_model;
    QIcon m_icon;
};

//-----------------------------------------------------------------------------

class AppNode : public Node
{
public:
    AppNode(Model *model = 0, Node *parent = 0, const QUrl &url = QUrl(), const QString &filePath = QString());

    QString name() const;
    QString category();
    QString comment() { return m_comment; }
    QString command() { return m_appCmd; }
    QIcon icon();
    QVariant data(const int column);
    void exec();

private:
    QString m_appName, m_appCmd, m_appIcon, m_category, m_comment, m_type;
};

//-----------------------------------------------------------------------------

namespace Worker
{
enum Task { Populate = 0, Generate = 1, Search = 2, GetApps = 3, NoTask = 4 };

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
    void populateApplications(const QString &appsPath, Node *node);
    void setCancelled(bool cancel);
    bool isCancelled() { QMutexLocker locker(&m_mutex); return m_isCancelled; }

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
