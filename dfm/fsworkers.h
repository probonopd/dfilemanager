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
#include "dataloader.h"
#include "devices.h"

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
    enum Child { Visible = 0, Hidden = 1, Filtered = 2, ChildrenTypeCount = 3 };
    typedef unsigned int Children;
    Node(FS::Model *model = 0, const QUrl &url = QUrl(), Node *parent = 0, const QString &filePath = QString());
    virtual ~Node();

    inline Model *model() const { return m_model; }
    Worker::Gatherer *gatherer() const;

    virtual QString name() const { return m_name; }
    bool rename(const QString &newName);
    inline QString filePath() const { return m_filePath; }

    int row() const;
    int rowOf(const Node *node) const;
    int childCount(Children children = Visible) const;
    void addChild(Node *node);
    Node *child(const int c, Children fromChildren = Visible) const;
    Node *child(const QString &name, const bool nameIsPath = true) const;
    Node *childFromUrl(const QUrl &url) const;
    bool hasChildren() const;
    void insertChild(Node *n, const int i);
    void deleteLater();

    void removeDeleted();
    void rePopulate();
    bool isPopulated() const;

    virtual QVariant data(const int column) const;
    virtual QIcon icon() const;
    virtual QString category() const;

    QString permissionsString() const;
    inline QString scheme() const { return url().scheme(); }
    QString mimeType() const;
    QString fileType() const;
    bool isExec() const;

    virtual void exec();

    void sort();
    int sortColumn() const;
    Qt::SortOrder sortOrder() const;

    struct Data *moreData() const;

    void setHiddenVisible(bool visible);
    bool showHidden() const;

    void setFilter(const QString &filter);
    inline QString filter() const { return m_filter; }

    void clearVisible();
    void removeChild(Node *node);

    Node *localNode(const QString &path, bool checkOnly = true);
    Node *nodeFromLocalPath(const QString &path, bool checkOnly = true);

    inline QUrl url() const { return m_url; }
    inline void setUrl(const QUrl &url) { m_url = url; }

    Node *parent() const;

private:
    mutable int m_isExe;
    mutable QMutex m_mutex;

    bool m_isPopulated, m_isDeleted;
    Nodes m_children[ChildrenTypeCount], m_toAdd;
    Node *m_parent;
    QString m_filePath, m_filter, m_name;
    QUrl m_url;
    Model *m_model;
    QIcon m_icon;
};

//-----------------------------------------------------------------------------

class AppNode : public Node
{
public:
    AppNode(Model *model = 0, Node *parent = 0, const QUrl &url = QUrl(), const QString &filePath = QString());

    QString name() const;
    QString category() const;
    inline QString comment() const { return m_comment; }
    inline QString command() const { return m_appCmd; }
    QIcon icon() const;
    QVariant data(const int column) const;
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
